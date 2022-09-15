/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * Copyright (c) 2018-2021 Sergey Ryazanov <ryazanov.s.a@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "atheepmgr.h"
#include "utils.h"
#include "eep_common.h"
#include "eep_9300.h"
#include "eep_9300_templates.h"

struct eep_9300_priv {
	int curr_ref_tpl;		/* Current reference EEPROM template */
	uint8_t unpack_buf[0x800];	/* Unpacking temporary data buffer */
	enum {
		DATA_SRC_NONE = 0,
		DATA_SRC_BLOB,
		DATA_SRC_EEPROM,
		DATA_SRC_OTP,
	} data_src;			/* Source of data in buffer */
	int init_data_max_size;		/* Position of data stream finish */
	int buf_is_be;			/* Is buf 16-bits word in big-endians */
	struct ar9300_eeprom eep;
};

#define EEPROM_DATA_LEN_9485	1088

#define AR9300_TEMPLATE_DESC(__name, __tpl)	\
	{ ar9300_tpl_ver_ ## __tpl, __name, &ar9300_ ## __tpl }

static const struct eeptemplate eep_9300_templates[] = {
	AR9300_TEMPLATE_DESC("default", default),
	AR9300_TEMPLATE_DESC("H112", h112),
	AR9300_TEMPLATE_DESC("H116", h116),
	AR9300_TEMPLATE_DESC("X112", x112),
	AR9300_TEMPLATE_DESC("X113", x113),
	{ 0, NULL }
};

static const uint8_t *ar9300_template_find_by_id(int id)
{
	const struct eeptemplate *tpl;

	for (tpl = eep_9300_templates; tpl->name; ++tpl)
		if (tpl->id == id)
			break;

	return tpl->data;
}

/**
 * Read data from EEPROM and fill internal buffer up to specified ammount of
 * bytes.
 */
static int ar9300_eep2buf(struct atheepmgr *aem, int bytes)
{
	int size = (bytes + 1) / 2;	/* Convert to 16 bits words */
	uint16_t *buf = aem->eep_buf;
	int addr;

	for (addr = aem->eep_len; addr < size; ++addr) {
		if (!EEP_READ(addr, &buf[addr])) {
			fprintf(stderr, "Unable to read EEPROM to buffer\n");
			return -1;
		}
	}

	if (addr > aem->eep_len)
		aem->eep_len = addr;

	return 0;
}

/**
 * Extract bytestream of specified length from the internal buffer as specified
 * offset.
 *
 * NB: we are reading bytes in a reverse direction from a stream of 16-bits
 * words. E.g. first two bytes of the output stream are extracted from a word
 * with a specified address, second two bytes of the output stream are extraced
 * from a predcessor word (with a lower address), and so on.
 */
static void ar9300_buf2bstr(struct atheepmgr *aem, int addr,
			    uint8_t *buffer, int count)
{
	struct eep_9300_priv *emp = aem->eepmap_priv;
	int i;

	if ((addr - count) < 0 || addr / 2  >= aem->eep_len) {
		fprintf(stderr, "Requested address not in range\n");
		memset(buffer, 0x00, count);
		return;
	}

	/*
	 * Endians of a buffer item word depends on a data source and a host
	 * machine endians: EEPROM is always in Little-endians format, while OTP
	 * is Native-endians. So we check the buffer endians flag to determine
	 * how we should handle buffer words and choose shift to select
	 * appropriate byte (low or high).
	 */

	for (i = addr; i > addr - count; --i) {
		int shift_bytes = emp->buf_is_be ? (i + 1) % 2 : i % 2;

		buffer[addr - i] = aem->eep_buf[i / 2] >> (8 * shift_bytes);
	}
}

/**
 * Read data from OTP mem and fill internal buffer up to specified ammount of
 * bytes.
 */
static int ar9300_otp2buf(struct atheepmgr *aem, int bytes)
{
	int size = (bytes + 1) & ~0x1;		/* 16-bits alignment */
	uint8_t *buf = (uint8_t *)aem->eep_buf;	/* Use as an array of bytes */
	int addr;

	/* NB: buffered data length is in 16-bits words */
	/* NB: fetch only unavailable portion of data (append buffer) */
	for (addr = aem->eep_len * 2; addr < size; ++addr) {
		if (!OTP_READ(addr, &buf[addr])) {
			fprintf(stderr, "Unable to read OTP to buffer\n");
			return -1;
		}
	}

	if (addr > aem->eep_len * 2)
		aem->eep_len = addr / 2;

	return 0;
}

static bool ar9300_check_header(void *data)
{
	uint32_t *word = data;
	return !(*word == 0 || *word == ~0);
}

static int ar9300_check_block_len(struct atheepmgr *aem, int max_len,
				  int blk_len)
{
	if (aem->con->caps & CON_CAP_HW) {
		if ((!AR_SREV_9485(aem) && blk_len >= 1024) ||
		    (AR_SREV_9485(aem) && blk_len > EEPROM_DATA_LEN_9485))
			return 0;
	}

	if (AR9300_COMP_HDR_LEN + blk_len + AR9300_COMP_CKSUM_LEN > max_len)
		return 0;

	return 1;
}

static int ar9300_check_eeprom_data(const struct ar9300_eeprom *eep)
{
	uint8_t txm, rxm;

	txm = eep->baseEepHeader.txrxMask >> 4;
	rxm = eep->baseEepHeader.txrxMask & 0x0f;
	if (txm == 0x00 || txm == 0xf || rxm == 0x0 || rxm == 0xf)
		return 0;

	if (!(eep->baseEepHeader.opCapFlags.opFlags & AR5416_OPFLAGS_11A) &&
	    !(eep->baseEepHeader.opCapFlags.opFlags & AR5416_OPFLAGS_11G))
		return 0;

	return 1;
}

static int ar9300_process_blocks(struct atheepmgr *aem, int cptr)
{
#define MSTATE	100
	struct eep_9300_priv *emp = aem->eepmap_priv;
	uint8_t *buf = emp->unpack_buf;
	int valid_blocks = 0;
	struct ar9300_comp_hdr hdr;
	uint16_t checksum, mchecksum;
	uint8_t *ptr;
	int it, res;

	emp->curr_ref_tpl = -1;	/* Reset reference template */

	for (it = 0; it < MSTATE; it++) {
		ar9300_buf2bstr(aem, cptr, buf, AR9300_COMP_HDR_LEN);

		if (!ar9300_check_header(buf))
			break;

		ar9300_comp_hdr_unpack(buf, &hdr);
		if (aem->verbose)
			printf("Found block at %x: comp=%d ref=%d length=%d major=%d minor=%d\n",
			       cptr, hdr.comp, hdr.ref, hdr.len, hdr.maj,
			       hdr.min);
		if (!ar9300_check_block_len(aem, cptr, hdr.len)) {
			if (aem->verbose)
				printf("Skipping bad header\n");
			cptr -= AR9300_COMP_HDR_LEN;
			continue;
		}

		ar9300_buf2bstr(aem, cptr, buf, AR9300_COMP_HDR_LEN +
				hdr.len + AR9300_COMP_CKSUM_LEN);

		checksum = ar9300_comp_cksum(buf + AR9300_COMP_HDR_LEN,
					     hdr.len);
		ptr = buf + AR9300_COMP_HDR_LEN + hdr.len;
		mchecksum = ptr[0] | (ptr[1] << 8);
		if (checksum != mchecksum) {
			if (aem->verbose)
				printf("Skipping block with bad checksum (got 0x%04x, expect 0x%04x)\n",
				       checksum, mchecksum);
			cptr -= AR9300_COMP_HDR_LEN;
			continue;
		}

		res = ar9300_compress_decision(aem, it, &hdr, aem->unpacked_buf,
					       buf + AR9300_COMP_HDR_LEN,
					       sizeof(emp->eep),
					       &emp->curr_ref_tpl,
					       ar9300_template_find_by_id);
		if (res == 0)
			valid_blocks++;

		cptr -= AR9300_COMP_HDR_LEN + hdr.len + AR9300_COMP_CKSUM_LEN;
	}

	emp->init_data_max_size = cptr;	/* Preserve for future usage */

	return valid_blocks ? 0 : -1;

#undef MSTATE
}

static bool eep_9300_load_blob(struct atheepmgr *aem)
{
	const int data_size = sizeof(struct ar9300_eeprom);
	struct eep_9300_priv *emp = aem->eepmap_priv;
	int res;

	if (aem->con->blob->getsize(aem) < data_size)
		return false;
	res = aem->con->blob->read(aem, aem->eep_buf, data_size);
	if (res != data_size) {
		fprintf(stderr, "Unable to read EEPROM blob\n");
		return false;
	}

	if (!ar9300_check_eeprom_data((struct ar9300_eeprom *)aem->eep_buf))
		return false;
	if (aem->verbose)
		printf("Found valid uncompressed EEPROM data\n");

	emp->data_src = DATA_SRC_BLOB;
	memcpy(&emp->eep, aem->eep_buf, sizeof(emp->eep));

	aem->eep_len = (data_size + 1) / 2;	/* Set actual EEPROM size */

	return true;
}

/*
 * Read the configuration data from the eeprom uncompress it if necessary.
 */
static bool eep_9300_load_eeprom(struct atheepmgr *aem, bool raw)
{
	struct eep_9300_priv *emp = aem->eepmap_priv;
	uint16_t magic;
	int cptr;

	emp->buf_is_be = 0;	/* EEPROM is always in Little-endians */
	aem->eep_len = 0;	/* Reset internal buffer contents */

	/* Check byteswaping requirements */
	if (!EEP_READ(AR5416_EEPROM_MAGIC_OFFSET, &magic)) {
		fprintf(stderr, "EEPROM magic read failed\n");
		return false;
	}
	if (bswap_16(magic) == AR5416_EEPROM_MAGIC) {
		if (aem->verbose)
			printf("Use byteswapped EEPROM I/O\n");
		aem->eep_io_swap = !aem->eep_io_swap;
	} else if (magic != AR5416_EEPROM_MAGIC) {
		return false;
	}

	if (aem->verbose)
		printf("EEPROM magic found\n");

	if (AR_SREV_9485(aem))
		cptr = AR9300_BASE_ADDR_4K;
	else if (AR_SREV_9330(aem))
		cptr = AR9300_BASE_ADDR_512;
	else
		cptr = AR9300_BASE_ADDR;

	if (aem->verbose)
		printf("Trying EEPROM access at Address 0x%04x\n", cptr);
	if (ar9300_eep2buf(aem, cptr) != 0)
		return false;
	if (ar9300_process_blocks(aem, cptr) == 0)
		goto found;

	cptr = AR9300_BASE_ADDR_512;
	if (aem->verbose)
		printf("Trying EEPROM access at Address 0x%04x\n", cptr);
	if (ar9300_process_blocks(aem, cptr) == 0)
		goto found;

	return false;

found:
	emp->data_src = DATA_SRC_EEPROM;
	aem->eep_len = (cptr + 1) / 2;	/* Set actual EEPROM size */
	aem->unpacked_len = sizeof(struct ar9300_eeprom);
	memcpy(&emp->eep, aem->unpacked_buf, sizeof(emp->eep));

	return true;
}

/*
 * Read the configuration data from the OTP memory uncompress it if necessary.
 */
static bool eep_9300_load_otp(struct atheepmgr *aem, bool raw)
{
	struct eep_9300_priv *emp = aem->eepmap_priv;
	int cptr;

	emp->buf_is_be = aem->host_is_be;	/* OTP utilize native-endians */
	aem->eep_len = 0;	/* Reset internal buffer contents */

	cptr = AR9300_BASE_ADDR;
	if (aem->verbose)
		printf("Trying OTP access at Address 0x%04x\n", cptr);
	if (ar9300_otp2buf(aem, cptr) != 0)
		return false;
	if (ar9300_process_blocks(aem, cptr) == 0)
		goto found;

	cptr = AR9300_BASE_ADDR_512;
	if (aem->verbose)
		printf("Trying OTP access at Address 0x%04x\n", cptr);
	if (ar9300_process_blocks(aem, cptr) == 0)
		goto found;

	return false;

found:
	emp->data_src = DATA_SRC_OTP;
	aem->eep_len = (cptr + 1) / 2;	/* Set actual EEPROM size */
	aem->unpacked_len = sizeof(struct ar9300_eeprom);
	memcpy(&emp->eep, aem->unpacked_buf, sizeof(emp->eep));

	return true;
}

static bool eep_9300_check(struct atheepmgr *aem)
{
	struct eep_9300_priv *emp = aem->eepmap_priv;
	struct ar9300_eeprom *eep = &emp->eep;
	struct ar9300_base_eep_hdr *pBase = &eep->baseEepHeader;
	int i;

	/* We perform all checks at data loading stage */
	if (emp->data_src == DATA_SRC_NONE)
		return false;

	if (!!(pBase->opCapFlags.eepMisc & AR5416_EEPMISC_BIG_ENDIAN) !=
	    aem->host_is_be) {
		struct ar9300_modal_eep_hdr *pModal;

		printf("EEPROM Endianness is not native.. Changing.\n");

		bswap_16_inplace(pBase->regDmn[0]);
		bswap_16_inplace(pBase->regDmn[1]);
		bswap_32_inplace(pBase->swreg);

		pModal = &eep->modalHeader5G;
		bswap_32_inplace(pModal->antCtrlCommon);
		bswap_32_inplace(pModal->antCtrlCommon2);
		for (i = 0; i < ARRAY_SIZE(pModal->antCtrlChain); ++i)
			bswap_16_inplace(pModal->antCtrlChain[i]);
		bswap_32_inplace(pModal->papdRateMaskHt20);
		bswap_32_inplace(pModal->papdRateMaskHt40);

		pModal = &eep->modalHeader2G;
		bswap_32_inplace(pModal->antCtrlCommon);
		bswap_32_inplace(pModal->antCtrlCommon2);
		for (i = 0; i < ARRAY_SIZE(pModal->antCtrlChain); ++i)
			bswap_16_inplace(pModal->antCtrlChain[i]);
		bswap_32_inplace(pModal->papdRateMaskHt20);
		bswap_32_inplace(pModal->papdRateMaskHt40);
	}

	return true;
}

static void eep_9300_dump_otp_init(struct ar9300_otp_init *ini,
				   size_t size)
{
	int i, maxregsnum;

	printf("Flags: 0x%08x\n", le32toh(ini->flags));
	printf("\n");

	EEP_PRINT_SUBSECT_NAME("Register(s) initialization data");

	maxregsnum = (size - offsetof(typeof(*ini), regs)) /
		     sizeof(ini->regs[0]);
	for (i = 0; i < maxregsnum; ++i) {
		if (!ini->regs[i].addr)
			break;
		printf("  %06X: %08X\n", le32toh(ini->regs[i].addr),
		       le32toh(ini->regs[i].val));
	}

	printf("\n");
}

static void eep_9300_dump_init_data(struct atheepmgr *aem)
{
	struct eep_9300_priv *emp = aem->eepmap_priv;

	EEP_PRINT_SECT_NAME("Chip init data");

	if (emp->data_src == DATA_SRC_BLOB) {
		printf("Blob has no chip initialization data\n");
		printf("\n");
	} else if (emp->data_src == DATA_SRC_EEPROM) {
		ar5416_dump_eep_init((struct ar5416_eep_init *)aem->eep_buf,
				     emp->init_data_max_size / 2);
	} else if (emp->data_src == DATA_SRC_OTP) {
		eep_9300_dump_otp_init((struct ar9300_otp_init *)aem->eep_buf,
				       emp->init_data_max_size);
	}
}

static void eep_9300_dump_base_header(struct atheepmgr *aem)
{
	struct eep_9300_priv *emp = aem->eepmap_priv;
	struct ar9300_eeprom *eep = &emp->eep;
	struct ar9300_base_eep_hdr *pBase = &eep->baseEepHeader;

	EEP_PRINT_SECT_NAME("EEPROM Base Header");

	printf("%-30s : %2d\n", "Version", eep->eepromVersion);
	printf("%-30s : 0x%04X\n", "RegDomain1", pBase->regDmn[0]);
	printf("%-30s : 0x%04X\n", "RegDomain2", pBase->regDmn[1]);
	printf("%-30s : %02X:%02X:%02X:%02X:%02X:%02X\n", "MacAddress",
			eep->macAddr[0], eep->macAddr[1], eep->macAddr[2],
			eep->macAddr[3], eep->macAddr[4], eep->macAddr[5]);
	printf("%-30s : 0x%04X\n", "TX Mask", pBase->txrxMask >> 4);
	printf("%-30s : 0x%04X\n", "RX Mask", pBase->txrxMask & 0x0f);
	printf("%-30s : %d\n", "Allow 5GHz",
			!!(pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11A));
	printf("%-30s : %d\n", "Allow 2GHz",
			!!(pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11G));
	printf("%-30s : %d\n", "Disable 2GHz HT20",
		!!(pBase->opCapFlags.opFlags & AR5416_OPFLAGS_N_2G_HT20));
	printf("%-30s : %d\n", "Disable 2GHz HT40",
		!!(pBase->opCapFlags.opFlags & AR5416_OPFLAGS_N_2G_HT40));
	printf("%-30s : %d\n", "Disable 5Ghz HT20",
		!!(pBase->opCapFlags.opFlags & AR5416_OPFLAGS_N_5G_HT20));
	printf("%-30s : %d\n", "Disable 5Ghz HT40",
		!!(pBase->opCapFlags.opFlags & AR5416_OPFLAGS_N_5G_HT40));
	printf("%-30s : %d\n", "Big Endian",
		!!(pBase->opCapFlags.eepMisc & AR5416_EEPMISC_BIG_ENDIAN));
	printf("%-30s : %x\n", "RF Silent", pBase->rfSilent);
	printf("%-30s : %x\n", "BT option", pBase->blueToothOptions);
	printf("%-30s : %x\n", "Device Cap", pBase->deviceCap);
	printf("%-30s : %s\n", "Device Type",
			sDeviceType[pBase->deviceType & 0x7]);
	printf("%-30s : %x\n", "Power Table Offset", pBase->pwrTableOffset);
	printf("%-30s : %x\n", "Tuning Caps1",
			pBase->params_for_tuning_caps[0]);
	printf("%-30s : %x\n", "Tuning Caps2",
			pBase->params_for_tuning_caps[1]);
	printf("%-30s : %x\n", "Enable Tx Temp Comp",
			!!(pBase->featureEnable & (1 << 0)));
	printf("%-30s : %d\n", "Enable Tx Volt Comp",
			!!(pBase->featureEnable & (1 << 1)));
	printf("%-30s : %d\n", "Enable fast clock",
			!!(pBase->featureEnable & (1 << 2)));
	printf("%-30s : %d\n", "Enable doubling",
			!!(pBase->featureEnable & (1 << 3)));
	printf("%-30s : %d\n", "Internal regulator",
			!!(pBase->featureEnable & (1 << 4)));
	printf("%-30s : %d\n", "Enable Paprd",
			!!(pBase->featureEnable & (1 << 5)));
	printf("%-30s : %d\n", "Driver Strength",
			!!(pBase->miscConfiguration & (1 << 0)));
	printf("%-30s : %d\n", "Quick Drop",
			!!(pBase->miscConfiguration & (1 << 1)));
	printf("%-30s : %d\n", "Chain mask Reduce",
			(pBase->miscConfiguration >> 0x3) & 0x1);
	printf("%-30s : %d\n", "Write enable Gpio",
			pBase->eepromWriteEnableGpio);
	printf("%-30s : %d\n", "WLAN Disable Gpio", pBase->wlanDisableGpio);
	printf("%-30s : %d\n", "WLAN LED Gpio", pBase->wlanLedGpio);
	printf("%-30s : %d\n", "Rx Band Select Gpio", pBase->rxBandSelectGpio);
	printf("%-30s : %d\n", "Tx Gain", pBase->txrxgain >> 4);
	printf("%-30s : %d\n", "Rx Gain", pBase->txrxgain & 0xf);
	printf("%-30s : %d\n", "SW Reg", pBase->swreg);

	printf("\nCustomer Data in hex:\n");
	hexdump_print(eep->custData, sizeof(eep->custData));

	printf("\n");
}

static void eep_9300_dump_modal_header(struct atheepmgr *aem)
{
#define PR(_token, _p, _val_fmt, _val)				\
	do {							\
		printf("%-23s %-8s", (_token), ":");		\
		if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11G) {	\
			pModal = &eep->modalHeader2G;		\
			printf("%s%-6"_val_fmt, _p, (_val));	\
		}						\
		if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11A) {	\
			pModal = &eep->modalHeader5G;		\
			printf("%8s%"_val_fmt"\n", _p, (_val)); \
		} else {					\
			printf("\n");				\
		}						\
	} while (0)

	struct eep_9300_priv *emp = aem->eepmap_priv;
	struct ar9300_eeprom *eep = &emp->eep;
	struct ar9300_base_eep_hdr *pBase = &eep->baseEepHeader;
	struct ar9300_modal_eep_hdr *pModal = NULL;

	EEP_PRINT_SECT_NAME("EEPROM Modal Header");

	if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11G)
		printf("%34s", "2G");
	if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11A)
		printf("%16s", "5G\n\n");
	else
		printf("\n\n");

	printf("%-23s %-8s", "Ant Chain 0", ":");
	if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11G) {
		pModal = &eep->modalHeader2G;
		printf("%-6d", pModal->antCtrlChain[0]);
	}
	if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11A) {
		pModal = &eep->modalHeader5G;
		printf("%10d\n", pModal->antCtrlChain[0]);
	} else
		 printf("\n");
	printf("%-23s %-8s", "Ant Chain 1", ":");
	if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11G) {
		pModal = &eep->modalHeader2G;
		printf("%-6d", pModal->antCtrlChain[1]);
	}
	if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11A) {
		pModal = &eep->modalHeader5G;
		printf("%10d\n", pModal->antCtrlChain[1]);
	} else
		 printf("\n");
	printf("%-23s %-8s", "Ant Chain 2", ":");
	if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11G) {
		pModal = &eep->modalHeader2G;
		printf("%-6d", pModal->antCtrlChain[2]);
	}
	if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11A) {
		pModal = &eep->modalHeader5G;
		printf("%10d\n", pModal->antCtrlChain[2]);
	} else
		 printf("\n");
	printf("%-23s %-8s", "Antenna Common", ":");
	if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11G) {
		pModal = &eep->modalHeader2G;
		printf("%-6d", pModal->antCtrlCommon);
	}
	if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11A) {
		pModal = &eep->modalHeader5G;
		printf("%10d\n", pModal->antCtrlCommon);
	} else
		 printf("\n");
	printf("%-23s %-8s", "Antenna Common2", ":");
	if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11G) {
		pModal = &eep->modalHeader2G;
		printf("%-6d", pModal->antCtrlCommon2);
	}
	if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11A) {
		pModal = &eep->modalHeader5G;
		printf("%10d\n", pModal->antCtrlCommon2);
	} else
		 printf("\n");
	PR("Antenna Gain", "", "d", pModal->antennaGain);
	PR("Switch Settling", "", "d", pModal->switchSettling);
	PR("xatten1DB Ch 0", "", "d", pModal->xatten1DB[0]);
	PR("xatten1DB Ch 1", "", "d", pModal->xatten1DB[1]);
	PR("xatten1DB Ch 2", "", "d", pModal->xatten1DB[2]);
	PR("xatten1Margin Chain 0", "", "d", pModal->xatten1Margin[0]);
	PR("xatten1Margin Chain 1", "", "d", pModal->xatten1Margin[1]);
	PR("xatten1Margin Chain 2", "", "d", pModal->xatten1Margin[2]);
	PR("Temp Slope", "", "d", pModal->tempSlope);
	PR("Volt Slope", "", "d", pModal->voltSlope);
	PR("spur Channels0", "", "d", pModal->spurChans[0]);
	PR("spur Channels1", "", "d", pModal->spurChans[1]);
	PR("spur Channels2", "", "d", pModal->spurChans[2]);
	PR("spur Channels3", "", "d", pModal->spurChans[3]);
	PR("spur Channels4", "", "d", pModal->spurChans[4]);
	PR("NF Thresh 0", "", "d", pModal->noiseFloorThreshCh[0]);
	PR("NF Thresh 1", "", "d", pModal->noiseFloorThreshCh[1]);
	PR("NF Thresh 2", "", "d", pModal->noiseFloorThreshCh[2]);
	PR("Quick Drop", "", "d", pModal->quick_drop);
	PR("TX end to xpa off", "", "d", pModal->txEndToXpaOff);
	PR("Xpa bias level", "", "d", pModal->xpaBiasLvl);
	PR("txFrameToDataStart", "", "d", pModal->txFrameToDataStart);
	PR("txFrameToPaOn", "", "d", pModal->txFrameToPaOn);
	PR("TX frame to xpa on", "", "d", pModal->txFrameToXpaOn);
	PR("TxClip", "", "d", pModal->txClip);
	PR("ADC Desired Size", "", "d", pModal->adcDesiredSize);
	PR("PAPD Rate Mask HT20", "0x", "x", pModal->papdRateMaskHt20);
	PR("PAPD Rate Mask HT40", "0x", "x", pModal->papdRateMaskHt40);

	printf("\n");

#undef PR
}

static void eep_9300_dump_pwr_cal(const uint8_t *piers, int maxpiers,
				  const struct ar9300_cal_data_per_freq_op_loop *data,
				  int is_2g, int chainmask)
{
	const struct ar9300_cal_data_per_freq_op_loop *d;
	int i, j;

	printf("               ");
	for (j = 0; j < AR9300_MAX_CHAINS; ++j) {
		if (!(chainmask & (1 << j)))
			continue;
		printf(".-------------- Chain %d -----------.", j);
	}
	printf("\n");
	printf("               ");
	for (j = 0; j < AR9300_MAX_CHAINS; ++j) {
		if (!(chainmask & (1 << j)))
			continue;
		printf("|        Tx        :       Rx      |");
	}
	printf("\n");

	printf("    Freq, MHz  ");
	for (j = 0; j < AR9300_MAX_CHAINS; ++j) {
		if (!(chainmask & (1 << j)))
			continue;
		printf(" PwrDelta Volt Temp    NF  Pwr Temp ");
	}
	printf("\n");

	for (i = 0; i < maxpiers; ++i) {
		printf("         %4u  ", FBIN2FREQ(piers[i], is_2g));
		for (j = 0; j < AR9300_MAX_CHAINS; ++j) {
			if (!(chainmask & (1 << j)))
				continue;
			d = &data[j * maxpiers + i];
			printf("    % 5.1f %4u %4u  %4d %4d %4u ",
			       (double)d->refPower / 2,
			       d->voltMeas, d->tempMeas,
			       d->rxNoisefloorCal, d->rxNoisefloorPower,
			       d->rxTempMeas);
		}
		printf("\n");
	}
}

static void eep_9300_dump_tgt_pwr(const uint8_t *freqs, int nfreqs,
				  const uint8_t *tgtpwr, int nrates,
				  const char * const rates[], int is_2g)
{
#define MARGIN		"    "
	int i, j;

	printf(MARGIN "%18s, MHz:", "Freq");
	for (j = 0; j < nfreqs; ++j)
		printf("  %4u", FBIN2FREQ(freqs[j], is_2g));
	printf("\n");
	printf(MARGIN "------------------------");
	for (j = 0; j < nfreqs; ++j)
		printf("  ----");
	printf("\n");

	for (i = 0; i < nrates; ++i) {
		printf(MARGIN "%18s, dBm:", rates[i]);
		for (j = 0; j < nfreqs; ++j)
			printf("  %4.1f", (double)tgtpwr[j * nrates + i] / 2);
		printf("\n");
	}
}

static const char * const eep_9300_rates_cck[4] = {
	"1-5 mbps (L)", "5 mbps (S)", "11 mbps (L)", "11 mbps (S)"
};

static const char * const eep_9300_rates_ofdm[4] = {
	"6-24 mbps", "36 mbps", "48 mbps", "54 mbps"
};

static const char * const eep_9300_rates_ht[14] = {
	"MCS 0,8,16", "MCS 1-3,9-11,17-19", "MCS 4", "MCS 5", "MCS 6", "MCS 7",
	"MCS 12", "MCS 13", "MCS 14", "MCS 15", "MCS 20", "MCS 21", "MCS 22",
	"MCS 23"
};

static void eep_9300_dump_power_info(struct atheepmgr *aem)
{
#define PR_PWR_CAL(__pref, __band, __is_2g)				\
	do {								\
		EEP_PRINT_SUBSECT_NAME(__pref " per-freq power cal. data");\
		eep_9300_dump_pwr_cal(eep->calFreqPier ## __band,	\
				      ARRAY_SIZE(eep->calFreqPier ## __band),\
				      &(eep->calPierData ## __band)[0][0],\
				      __is_2g,				\
				      eep->baseEepHeader.txrxMask >> 4);\
		printf("\n");						\
	} while (0);
#define PR_TARGET_POWER(__pref, __mod, __rates, __is_2g)		\
	do {								\
		EEP_PRINT_SUBSECT_NAME(__pref " per-rate target power");\
		eep_9300_dump_tgt_pwr(eep->calTarget_freqbin_ ## __mod,\
				      ARRAY_SIZE(eep->calTarget_freqbin_ ## __mod),\
				      (void *)(eep->calTargetPower ## __mod),\
				      ARRAY_SIZE((eep->calTargetPower ## __mod)[0].tPow2x),\
				      __rates, __is_2g);		\
		printf("\n");						\
	} while (0);
#define PR_CTL(__pref, __band, __is_2g)					\
	do {								\
		EEP_PRINT_SUBSECT_NAME(__pref " CTL data");		\
		ar9300_dump_ctl(eep->ctlIndex_ ## __band,		\
				(uint8_t *)eep->ctl_freqbin_ ## __band,	\
				(uint8_t *)eep->ctlPowerData_ ## __band,\
				AR9300_NUM_CTLS_ ## __band,		\
				AR9300_NUM_BAND_EDGES_ ## __band, __is_2g);\
	} while (0);
	struct eep_9300_priv *emp = aem->eepmap_priv;
	struct ar9300_eeprom *eep = &emp->eep;

	EEP_PRINT_SECT_NAME("EEPROM Power Info");

	if (eep->baseEepHeader.opCapFlags.opFlags & AR5416_OPFLAGS_11G)
		PR_PWR_CAL("2 GHz", 2G, 1);
	if (eep->baseEepHeader.opCapFlags.opFlags & AR5416_OPFLAGS_11A)
		PR_PWR_CAL("5 GHz", 5G, 0);

	if (eep->baseEepHeader.opCapFlags.opFlags & AR5416_OPFLAGS_11G) {
		PR_TARGET_POWER("2 GHz CCK", Cck, eep_9300_rates_cck, 1);
		PR_TARGET_POWER("2 GHz OFDM", 2G, eep_9300_rates_ofdm, 1);
		PR_TARGET_POWER("2 GHz HT20", 2GHT20, eep_9300_rates_ht, 1);
		PR_TARGET_POWER("2 GHz HT40", 2GHT40, eep_9300_rates_ht, 1);
	}
	if (eep->baseEepHeader.opCapFlags.opFlags & AR5416_OPFLAGS_11A) {
		PR_TARGET_POWER("5 GHz OFDM", 5G, eep_9300_rates_ofdm, 0);
		PR_TARGET_POWER("5 GHz HT20", 5GHT20, eep_9300_rates_ht, 0);
		PR_TARGET_POWER("5 GHz HT40", 5GHT40, eep_9300_rates_ht, 0);
	}

	if (eep->baseEepHeader.opCapFlags.opFlags & AR5416_OPFLAGS_11G)
		PR_CTL("2 GHz", 2G, 1);
	if (eep->baseEepHeader.opCapFlags.opFlags & AR5416_OPFLAGS_11A)
		PR_CTL("5 GHz", 5G, 0);

#undef PR_CTL
#undef PR_TARGET_POWER
#undef PR_PWR_CAL
}

static bool eep_9300_update_eeprom(struct atheepmgr *aem, int param,
				   const void *data)
{
	struct eep_9300_priv *emp = aem->eepmap_priv;
	struct ar9300_eeprom *eep = &emp->eep;
	uint16_t *buf = aem->eep_buf;
	int data_pos, data_len = 0, addr;

	if (emp->data_src != DATA_SRC_BLOB) {
		fprintf(stderr, "Updation is supported for uncompressed data only\n");
		return false;
	}

	switch (param) {
	case EEP_UPDATE_MAC:
		data_pos = EEP_FIELD_OFFSET(macAddr);
		data_len = EEP_FIELD_SIZE(macAddr);
		memcpy(&buf[data_pos], data, data_len * sizeof(uint16_t));
		break;
	default:
		fprintf(stderr, "Internal error: unknown parameter Id\n");
		return false;
	}

	/* Store updated data */
	for (addr = data_pos; addr < (data_pos + data_len); ++addr) {
		if (!EEP_WRITE(addr, buf[addr])) {
			fprintf(stderr, "Unable to write EEPROM data at 0x%04x\n",
				addr);
			return false;
		}
	}

	return true;
}

const struct eepmap eepmap_9300 = {
	.name = "9300",
	.desc = "EEPROM map for modern .11n chips (AR93xx/AR94xx/AR95xx/etc.)",
	.chip_regs = {
		.srev = 0x4020,
	},
	.priv_data_sz = sizeof(struct eep_9300_priv),
	.eep_buf_sz = AR9300_EEPROM_SIZE / sizeof(uint16_t),
	.unpacked_buf_sz = sizeof(struct ar9300_eeprom),
	.templates = eep_9300_templates,
	.load_blob = eep_9300_load_blob,
	.load_eeprom = eep_9300_load_eeprom,
	.load_otp = eep_9300_load_otp,
	.check_eeprom = eep_9300_check,
	.dump = {
		[EEP_SECT_INIT] = eep_9300_dump_init_data,
		[EEP_SECT_BASE] = eep_9300_dump_base_header,
		[EEP_SECT_MODAL] = eep_9300_dump_modal_header,
		[EEP_SECT_POWER] = eep_9300_dump_power_info,
	},
	.update_eeprom = eep_9300_update_eeprom,
	.params_mask = BIT(EEP_UPDATE_MAC)
#ifdef CONFIG_I_KNOW_WHAT_I_AM_DOING
#endif
	,
};
