/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * Copyright (c) 2018 Sergey Ryazanov <ryazanov.s.a@gmail.com>
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
#include "eep_9300.h"
#include "eep_9300_templates.h"

/* Uncompressed EEPROM block header */
struct eep_9300_blk_hdr {
	int comp;	/* Compression type */
	int ref;	/* Reference EEPROM data */
	int len;	/* Block length */
	int maj;
	int min;
};

struct eep_9300_priv {
	int valid_blocks;
	struct ar9300_eeprom eep;
};

#define COMP_HDR_LEN 4
#define COMP_CKSUM_LEN 2

#define EEPROM_DATA_LEN_9485	1088

static const struct ar9300_eeprom * const ar9300_eep_templates[] = {
	&ar9300_default,
	&ar9300_x112,
	&ar9300_h116,
	&ar9300_h112,
	&ar9300_x113,
};

static const struct ar9300_eeprom *ar9300_eeprom_struct_find_by_id(int id)
{
	int it;

	for (it = 0; it < ARRAY_SIZE(ar9300_eep_templates); it++)
		if (ar9300_eep_templates[it]->templateVersion == id)
			return ar9300_eep_templates[it];
	return NULL;
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
 * NB: we are reading the bytes in reverse order for a stream of 16-bit words.
 * E.g. first two bytes of the output stream extracted from word with specified
 * address, second two bytes of the output stream extraced from buffered word
 * with lower address, and so on.
 */
static void ar9300_buf2bstr(struct atheepmgr *aem, int addr,
			    uint8_t *buffer, int count)
{
	int i;

	if ((addr - count) < 0 || addr / 2  >= aem->eep_len) {
		fprintf(stderr, "Requested address not in range\n");
		memset(buffer, 0x00, count);
		return;
	}

	/*
	 * We're reading the bytes in reverse order from a little-endian
	 * word stream, an even address means we only use the lower half of
	 * the 16-bit word at that address
	 */

	for (i = addr; i > addr - count; --i)
		buffer[addr - i] = aem->eep_buf[i / 2] >> (8 * (i % 2));
}

/**
 * Since the AR93xx EEPROM layout does not contain any predefined value
 * (like a magic number), we can not determine at the EEPROM contents
 * loading stage whether to do the byte-swaping or not.
 *
 * So, this routine is used to byte-swap a data that are already loaded
 * to the buffer.
 */
static void ar9300_buf_byteswap(struct atheepmgr *aem)
{
	int i;

	for (i = 0; i < aem->eep_len; ++i)
		aem->eep_buf[i] = bswap_16(aem->eep_buf[i]);
}

static bool ar9300_otp_read_word(struct atheepmgr *aem, int addr, uint32_t *data)
{
	REG_READ(AR9300_OTP_BASE + (4 * addr));

	if (!hw_wait(aem, AR9300_OTP_STATUS, AR9300_OTP_STATUS_TYPE,
		     AR9300_OTP_STATUS_VALID, 1000))
		return false;

	*data = REG_READ(AR9300_OTP_READ_DATA);
	return true;
}

/**
 * Read data from OTP mem and fill internal buffer up to specified ammount of
 * bytes.
 */
static int ar9300_otp2buf(struct atheepmgr *aem, int bytes)
{
	int size = (bytes + 3) / 4;	/* Convert to 32 bits words */
	uint16_t *buf = aem->eep_buf;
	uint32_t word;
	int addr;

	for (addr = aem->eep_len / 2; addr < size; ++addr) {
		if (!ar9300_otp_read_word(aem, addr, &word)) {
			fprintf(stderr, "Unable to read OTP to buffer\n");
			return -1;
		}
		/**
		 * Mimic EEPROM when placing 32-bit OTP word to the buffer,
		 * which is array of 16-bit words. Assume we are on
		 * little-endian platform.
		 */
		buf[addr * 2 + 0] = word & 0xffff;
		buf[addr * 2 + 1] = word >> 16;
	}

	if (addr > aem->eep_len / 2)
		aem->eep_len = addr * 2;

	return 0;
}

static void ar9300_comp_hdr_unpack(uint8_t *best, struct eep_9300_blk_hdr *blkh)
{
	unsigned long value[4];

	value[0] = best[0];
	value[1] = best[1];
	value[2] = best[2];
	value[3] = best[3];
	blkh->comp = ((value[0] >> 5) & 0x0007);
	blkh->ref = (value[0] & 0x001f) | ((value[1] >> 2) & 0x0020);
	blkh->len = ((value[1] << 4) & 0x07f0) | ((value[2] >> 4) & 0x000f);
	blkh->maj = (value[2] & 0x000f);
	blkh->min = (value[3] & 0x00ff);
}

static uint16_t ar9300_comp_cksum(uint8_t *data, int dsize)
{
	int it, checksum = 0;

	for (it = 0; it < dsize; it++) {
		checksum += data[it];
		checksum &= 0xffff;
	}

	return checksum;
}

static bool ar9300_uncompress_block(struct atheepmgr *aem, uint8_t *mptr,
				    int mdataSize, uint8_t *block, int size)
{
	int it;
	int spot;
	int offset;
	int length;

	spot = 0;

	for (it = 0; it < size; it += (length+2)) {
		offset = block[it];
		offset &= 0xff;
		spot += offset;
		length = block[it+1];
		length &= 0xff;

		if (length > 0 && spot >= 0 && spot+length <= mdataSize) {
			if (aem->verbose)
				printf("Restore at %d: spot=%d offset=%d length=%d\n",
				       it, spot, offset, length);
			memcpy(&mptr[spot], &block[it+2], length);
			spot += length;
		} else if (length > 0) {
			fprintf(stderr,
				"Bad restore at %d: spot=%d offset=%d length=%d\n",
				it, spot, offset, length);
			return false;
		}
	}
	return true;
}

static int ar9300_compress_decision(struct atheepmgr *aem, int it,
				    struct eep_9300_blk_hdr *blkh,
				    uint8_t *mptr, uint8_t *word,
				    int mdata_size)
{
	const struct ar9300_eeprom *eep = NULL;
	bool res;

	switch (blkh->comp) {
	case _CompressNone:
		if (blkh->len != mdata_size) {
			fprintf(stderr,
				"EEPROM structure size mismatch memory=%d eeprom=%d\n",
				mdata_size, blkh->len);
			return -1;
		}
		memcpy(mptr, word + COMP_HDR_LEN, blkh->len);
		if (aem->verbose)
			printf("restored eeprom %d: uncompressed, length %d\n",
			       it, blkh->len);
		break;
	case _CompressBlock:
		if (blkh->ref != 0) {
			eep = ar9300_eeprom_struct_find_by_id(blkh->ref);
			if (eep == NULL) {
				fprintf(stderr,
					"can't find reference eeprom struct %d\n",
					blkh->ref);
				return -1;
			}
			memcpy(mptr, eep, mdata_size);
		}
		if (aem->verbose)
			printf("Restore eeprom %d: block, reference %d, length %d\n",
			       it, blkh->ref, blkh->len);
		res = ar9300_uncompress_block(aem, mptr, mdata_size,
					      (word + COMP_HDR_LEN), blkh->len);
		if (!res)
			return -1;
		break;
	default:
		fprintf(stderr, "unknown compression code %d\n", blkh->comp);
		return -1;
	}
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

	if (COMP_HDR_LEN + blk_len + COMP_CKSUM_LEN > max_len)
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

static int ar9300_process_blocks(struct atheepmgr *aem, uint8_t *buf,
				 int cptr)
{
#define MSTATE 100
	struct eep_9300_priv *emp = aem->eepmap_priv;
	int prev_valid_blocks = emp->valid_blocks;
	struct eep_9300_blk_hdr blkh;
	uint16_t checksum, mchecksum;
	uint8_t *ptr;
	int it, res;

	for (it = 0; it < MSTATE; it++) {
		ar9300_buf2bstr(aem, cptr, buf, COMP_HDR_LEN);

		if (!ar9300_check_header(buf))
			break;

		ar9300_comp_hdr_unpack(buf, &blkh);
		if (aem->verbose)
			printf("Found block at %x: comp=%d ref=%d length=%d major=%d minor=%d\n",
			       cptr, blkh.comp, blkh.ref, blkh.len, blkh.maj,
			       blkh.min);
		if (!ar9300_check_block_len(aem, cptr, blkh.len)) {
			if (aem->verbose)
				printf("Skipping bad header\n");
			cptr -= COMP_HDR_LEN;
			continue;
		}

		ar9300_buf2bstr(aem, cptr, buf,
				COMP_HDR_LEN + blkh.len + COMP_CKSUM_LEN);

		checksum = ar9300_comp_cksum(&buf[COMP_HDR_LEN], blkh.len);
		ptr = &buf[COMP_HDR_LEN + blkh.len];
		mchecksum = ptr[0] | (ptr[1] << 8);
		if (checksum != mchecksum) {
			if (aem->verbose)
				printf("Skipping block with bad checksum (got 0x%04x, expect 0x%04x)\n",
				       checksum, mchecksum);
			cptr -= COMP_HDR_LEN;
			continue;
		}

		res = ar9300_compress_decision(aem, it, &blkh,
					       (uint8_t *)&emp->eep, buf,
					       sizeof(emp->eep));
		if (res == 0)
			emp->valid_blocks++;

		cptr -= COMP_HDR_LEN + blkh.len + COMP_CKSUM_LEN;
	}

	return prev_valid_blocks == emp->valid_blocks ? -1 : 0;

#undef MSTATE
}

/*
 * Read the configuration data from the eeprom uncompress it if necessary.
 */
static bool eep_9300_fill(struct atheepmgr *aem)
{
	struct eep_9300_priv *emp = aem->eepmap_priv;
	int cptr;
	uint8_t *word;
	int bswap = aem->eep_io_swap;

	word = calloc(1, 2048);
	if (!word) {
		fprintf(stderr, "Unable to allocate temporary buffer\n");
		return false;
	}

	memcpy(&emp->eep, &ar9300_default, sizeof(emp->eep));

parse_eeprom:
	if (ar9300_eep2buf(aem, sizeof(struct ar9300_eeprom)) != 0)
		goto fail;
	if (ar9300_check_eeprom_data((struct ar9300_eeprom *)aem->eep_buf)) {
		if (aem->verbose)
			printf("Found valid uncompressed EEPROM data\n");
		cptr = sizeof(emp->eep);
		memcpy(&emp->eep, aem->eep_buf, sizeof(emp->eep));
		emp->valid_blocks++;
		goto found;
	}

	if (AR_SREV_9485(aem))
		cptr = AR9300_BASE_ADDR_4K;
	else if (AR_SREV_9330(aem))
		cptr = AR9300_BASE_ADDR_512;
	else
		cptr = AR9300_BASE_ADDR;

	if (aem->verbose)
		printf("Trying EEPROM access at Address 0x%04x\n", cptr);
	if (ar9300_eep2buf(aem, cptr) != 0)
		goto fail;
	if (ar9300_process_blocks(aem, word, cptr) == 0)
		goto found;

	cptr = AR9300_BASE_ADDR_512;
	if (aem->verbose)
		printf("Trying EEPROM access at Address 0x%04x\n", cptr);
	if (ar9300_process_blocks(aem, word, cptr) == 0)
		goto found;

	if (aem->eep_io_swap == bswap) {
		if (aem->verbose)
			printf("Try to byteswap EEPROM contents\n");
		aem->eep_io_swap = !aem->eep_io_swap;
		ar9300_buf_byteswap(aem);
		goto parse_eeprom;	/* Reparse EEPROM contents */
	}

	/* Avoid OTP touching if no real access to the hardware. */
	if (!(aem->con->caps & CON_CAP_HW))
		goto fail;

	aem->eep_len = 0;	/* Reset internal buffer contents */

	cptr = AR9300_BASE_ADDR;
	if (aem->verbose)
		printf("Trying OTP access at Address 0x%04x\n", cptr);
	if (ar9300_otp2buf(aem, cptr) != 0)
		goto fail;
	if (ar9300_process_blocks(aem, word, cptr) == 0)
		goto found;

	cptr = AR9300_BASE_ADDR_512;
	if (aem->verbose)
		printf("Trying OTP access at Address 0x%04x\n", cptr);
	if (ar9300_process_blocks(aem, word, cptr) == 0)
		goto found;

	goto fail;

found:
	aem->eep_len = (cptr + 1) / 2;	/* Set actual EEPROM size */
	free(word);
	return true;

fail:
	free(word);
	return false;
}

static int eep_9300_check(struct atheepmgr *aem)
{
	struct eep_9300_priv *emp = aem->eepmap_priv;

	return emp->valid_blocks ? 1 : 0;
}

static void eep_9300_dump_base_header(struct atheepmgr *aem)
{
	struct eep_9300_priv *emp = aem->eepmap_priv;
	struct ar9300_eeprom *eep = &emp->eep;
	struct ar9300_base_eep_hdr *pBase = &eep->baseEepHeader;

	EEP_PRINT_SECT_NAME("EEPROM Base Header");

	printf("%-30s : %2d\n", "Version", eep->eepromVersion);
	printf("%-30s : 0x%04X\n", "RegDomain1", le16toh(pBase->regDmn[0]));
	printf("%-30s : 0x%04X\n", "RegDomain2", le16toh(pBase->regDmn[1]));
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
			!!(pBase->opCapFlags.eepMisc & 0x01));
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
	printf("%-30s : %d\n", "SW Reg", le32toh(pBase->swreg));

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
		printf("%-6d", le16toh(pModal->antCtrlChain[0]));
	}
	if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11A) {
		pModal = &eep->modalHeader5G;
		printf("%10d\n", le16toh(pModal->antCtrlChain[0]));
	} else
		 printf("\n");
	printf("%-23s %-8s", "Ant Chain 1", ":");
	if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11G) {
		pModal = &eep->modalHeader2G;
		printf("%-6d", le16toh(pModal->antCtrlChain[1]));
	}
	if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11A) {
		pModal = &eep->modalHeader5G;
		printf("%10d\n", le16toh(pModal->antCtrlChain[1]));
	} else
		 printf("\n");
	printf("%-23s %-8s", "Ant Chain 2", ":");
	if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11G) {
		pModal = &eep->modalHeader2G;
		printf("%-6d", le16toh(pModal->antCtrlChain[2]));
	}
	if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11A) {
		pModal = &eep->modalHeader5G;
		printf("%10d\n", le16toh(pModal->antCtrlChain[2]));
	} else
		 printf("\n");
	printf("%-23s %-8s", "Antenna Common", ":");
	if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11G) {
		pModal = &eep->modalHeader2G;
		printf("%-6d", le32toh(pModal->antCtrlCommon));
	}
	if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11A) {
		pModal = &eep->modalHeader5G;
		printf("%10d\n", le32toh(pModal->antCtrlCommon));
	} else
		 printf("\n");
	printf("%-23s %-8s", "Antenna Common2", ":");
	if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11G) {
		pModal = &eep->modalHeader2G;
		printf("%-6d", le32toh(pModal->antCtrlCommon2));
	}
	if (pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11A) {
		pModal = &eep->modalHeader5G;
		printf("%10d\n", le32toh(pModal->antCtrlCommon2));
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
		printf(".------------- Chain %d ----------.", j);
	}
	printf("\n");
	printf("               ");
	for (j = 0; j < AR9300_MAX_CHAINS; ++j) {
		if (!(chainmask & (1 << j)))
			continue;
		printf("|       Tx       :       Rx      |");
	}
	printf("\n");

	printf("    Freq, MHz  ");
	for (j = 0; j < AR9300_MAX_CHAINS; ++j) {
		if (!(chainmask & (1 << j)))
			continue;
		printf(" RefPwr Volt Temp    NF  Pwr Temp ");
	}
	printf("\n");

	for (i = 0; i < maxpiers; ++i) {
		printf("         %4u  ", FBIN2FREQ(piers[i], is_2g));
		for (j = 0; j < AR9300_MAX_CHAINS; ++j) {
			if (!(chainmask & (1 << j)))
				continue;
			d = &data[j * maxpiers + i];
			printf("   %4d %4u %4u  %4d %4d %4u ", d->refPower,
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
}

const struct eepmap eepmap_9300 = {
	.name = "9300",
	.desc = "EEPROM map for modern .11n chips (AR93xx/AR64xx/AR95xx/etc.)",
	.priv_data_sz = sizeof(struct eep_9300_priv),
	.eep_buf_sz = AR9300_EEPROM_SIZE / sizeof(uint16_t),
	.fill_eeprom = eep_9300_fill,
	.check_eeprom = eep_9300_check,
	.dump = {
		[EEP_SECT_BASE] = eep_9300_dump_base_header,
		[EEP_SECT_MODAL] = eep_9300_dump_modal_header,
		[EEP_SECT_POWER] = eep_9300_dump_power_info,
	},
};
