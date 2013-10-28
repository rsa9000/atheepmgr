/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * Copyright (c) 2013 Sergey Ryazanov <ryazanov.s.a@gmail.com>
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

#include "edump.h"
#include "eep_9285.h"

struct eep_9285_priv {
	union {
		struct ar5416_init ini;
		uint16_t init_data[AR9285_DATA_START_LOC];
	};
	struct ar9285_eeprom eep;
};

static int eep_9285_get_ver(struct eep_9285_priv *emp)
{
	return ((emp->eep.baseEepHeader.version >> 12) & 0xF);
}

static int eep_9285_get_rev(struct eep_9285_priv *emp)
{
	return ((emp->eep.baseEepHeader.version) & 0xFFF);
}

static bool eep_9285_fill(struct edump *edump)
{
	struct eep_9285_priv *emp = edump->eepmap_priv;
	uint16_t *eep_data = (uint16_t *)&emp->eep;
	uint16_t *eep_init = (uint16_t *)&emp->ini;
	uint16_t *buf = edump->eep_buf;
	uint16_t magic;
	int addr;

	/* Check byteswaping requirements */
	if (!EEP_READ(AR5416_EEPROM_MAGIC_OFFSET, &magic)) {
		fprintf(stderr, "EEPROM magic read failed\n");
		return false;
	}
	if (bswap_16(magic) == AR5416_EEPROM_MAGIC)
		edump->eep_io_swap = !edump->eep_io_swap;

	/* Read to the intermediate buffer */
	for (addr = 0; addr < AR9285_DATA_START_LOC + AR9285_DATA_SZ; ++addr) {
		if (!EEP_READ(addr, &buf[addr])) {
			fprintf(stderr, "Unable to read EEPROM to buffer\n");
			return false;
		}
	}

	/* Copy from buffer to the Init data */
	for (addr = 0; addr < AR9285_DATA_START_LOC; ++addr)
		eep_init[addr] = buf[addr];

	/* Copy from buffer to the EEPROM structure */
	for (addr = 0; addr < AR9285_DATA_SZ; ++addr)
		eep_data[addr] = buf[AR9285_DATA_START_LOC + addr];

	return true;
}

static bool eep_9285_check(struct edump *edump)
{
	struct eep_9285_priv *emp = edump->eepmap_priv;
	struct ar5416_init *ini = &emp->ini;
	struct ar9285_eeprom *eep = &emp->eep;
	const uint16_t *buf = edump->eep_buf;
	uint16_t sum;
	int i, el;

	if (ini->magic != AR5416_EEPROM_MAGIC) {
		fprintf(stderr, "Invalid EEPROM Magic 0x%04x, expected 0x%04x\n",
			ini->magic, AR5416_EEPROM_MAGIC);
		return false;
	}

	if (!!(eep->baseEepHeader.eepMisc & AR5416_EEPMISC_BIG_ENDIAN) !=
	    edump->host_is_be) {
		uint32_t integer;
		uint16_t word;

		printf("EEPROM Endianness is not native.. Changing\n");

		word = bswap_16(eep->baseEepHeader.length);
		eep->baseEepHeader.length = word;

		word = bswap_16(eep->baseEepHeader.checksum);
		eep->baseEepHeader.checksum = word;

		word = bswap_16(eep->baseEepHeader.version);
		eep->baseEepHeader.version = word;

		word = bswap_16(eep->baseEepHeader.regDmn[0]);
		eep->baseEepHeader.regDmn[0] = word;

		word = bswap_16(eep->baseEepHeader.regDmn[1]);
		eep->baseEepHeader.regDmn[1] = word;

		word = bswap_16(eep->baseEepHeader.rfSilent);
		eep->baseEepHeader.rfSilent = word;

		word = bswap_16(eep->baseEepHeader.blueToothOptions);
		eep->baseEepHeader.blueToothOptions = word;

		word = bswap_16(eep->baseEepHeader.deviceCap);
		eep->baseEepHeader.deviceCap = word;

		integer = bswap_32(eep->modalHeader.antCtrlCommon);
		eep->modalHeader.antCtrlCommon = integer;

		for (i = 0; i < AR9285_MAX_CHAINS; i++) {
			integer = bswap_32(eep->modalHeader.antCtrlChain[i]);
			eep->modalHeader.antCtrlChain[i] = integer;
		}

		for (i = 0; i < AR_EEPROM_MODAL_SPURS; i++) {
			word = bswap_16(eep->modalHeader.spurChans[i].spurChan);
			eep->modalHeader.spurChans[i].spurChan = word;
		}
	}

	if (eep_9285_get_ver(emp) != AR5416_EEP_VER ||
	    eep_9285_get_rev(emp) < AR5416_EEP_NO_BACK_VER) {
		fprintf(stderr, "Bad EEPROM version 0x%04x (%d.%d)\n",
			eep->baseEepHeader.version, eep_9285_get_ver(emp),
			eep_9285_get_rev(emp));
		return false;
	}

	el = eep->baseEepHeader.length / sizeof(uint16_t);
	if (el > AR9285_DATA_SZ)
		el = AR9285_DATA_SZ;

	sum = eep_calc_csum(&buf[AR9285_DATA_START_LOC], el);
	if (sum != 0xffff) {
		fprintf(stderr, "Bad EEPROM checksum 0x%04x\n", sum);
		return false;
	}

	return true;
}

static void eep_9285_dump_init_data(struct edump *edump)
{
	struct eep_9285_priv *emp = edump->eepmap_priv;
	struct ar5416_init *ini = &emp->ini;
	uint16_t magic = le16toh(ini->magic);
	uint16_t prot = le16toh(ini->prot);
	uint16_t iptr = le16toh(ini->iptr);
	int i, maxregsnum;

	EEP_PRINT_SECT_NAME("EEPROM Init data");

	printf("%-20s : 0x%04X\n", "Magic", magic);
	for (i = 0; i < 8; ++i)
		printf("Region%d access       : %s\n", i,
		       sAccessType[(prot >> (i * 2)) & 0x3]);
	printf("%-20s : 0x%04X\n", "Regs init data ptr", iptr);
	printf("\n");

	EEP_PRINT_SUBSECT_NAME("Register initialization data");

	maxregsnum = (sizeof(emp->init_data) - offsetof(typeof(*ini), regs)) /
		     sizeof(ini->regs[0]);
	for (i = 0; i < maxregsnum; ++i) {
		if (ini->regs[i].addr == 0xffff)
			break;
		printf("  %04X: %08X\n", le16toh(ini->regs[i].addr),
		       le32toh(ini->regs[i].val));
	}

	printf("\n");
}

static void eep_9285_dump_base_header(struct edump *edump)
{
	struct eep_9285_priv *emp = edump->eepmap_priv;
	struct ar9285_eeprom *eep = &emp->eep;
	struct ar9285_base_eep_hdr *pBase = &eep->baseEepHeader;
	uint16_t i;

	EEP_PRINT_SECT_NAME("EEPROM Base Header");

	printf("%-30s : %2d\n", "Major Version",
	       pBase->version >> 12);
	printf("%-30s : %2d\n", "Minor Version",
	       pBase->version & 0xFFF);
	printf("%-30s : 0x%04X\n", "Checksum",
	       pBase->checksum);
	printf("%-30s : 0x%04X\n", "Length",
	       pBase->length);
	printf("%-30s : 0x%04X\n", "RegDomain1",
	       pBase->regDmn[0]);
	printf("%-30s : 0x%04X\n", "RegDomain2",
	       pBase->regDmn[1]);
	printf("%-30s : %02X:%02X:%02X:%02X:%02X:%02X\n",
	       "MacAddress",
	       pBase->macAddr[0], pBase->macAddr[1], pBase->macAddr[2],
	       pBase->macAddr[3], pBase->macAddr[4], pBase->macAddr[5]);
	printf("%-30s : 0x%04X\n",
	       "TX Mask", pBase->txMask);
	printf("%-30s : 0x%04X\n",
	       "RX Mask", pBase->rxMask);
	if (pBase->rfSilent & AR5416_RFSILENT_ENABLED)
		printf("%-30s : GPIO:%u Pol:%c\n", "RfSilent",
		       MS(pBase->rfSilent, AR5416_RFSILENT_GPIO_SEL),
		       MS(pBase->rfSilent, AR5416_RFSILENT_POLARITY)?'H':'L');
	else
		printf("%-30s : disabled\n", "RfSilent");
	printf("%-30s : %d\n",
	       "OpFlags(5GHz)",
	       !!(pBase->opCapFlags & AR5416_OPFLAGS_11A));
	printf("%-30s : %d\n",
	       "OpFlags(2GHz)",
	       !!(pBase->opCapFlags & AR5416_OPFLAGS_11G));
	printf("%-30s : %d\n",
	       "OpFlags(Disable 2GHz HT20)",
	       !!(pBase->opCapFlags & AR5416_OPFLAGS_N_2G_HT20));
	printf("%-30s : %d\n",
	       "OpFlags(Disable 2GHz HT40)",
	       !!(pBase->opCapFlags & AR5416_OPFLAGS_N_2G_HT40));
	printf("%-30s : %d\n",
	       "OpFlags(Disable 5Ghz HT20)",
	       !!(pBase->opCapFlags & AR5416_OPFLAGS_N_5G_HT20));
	printf("%-30s : %d\n",
	       "OpFlags(Disable 5Ghz HT40)",
	       !!(pBase->opCapFlags & AR5416_OPFLAGS_N_5G_HT40));
	printf("%-30s : %d\n",
	       "Big Endian",
	       !!(pBase->eepMisc & AR5416_EEPMISC_BIG_ENDIAN));
	printf("%-30s : %d\n",
	       "Cal Bin Major Ver",
	       (pBase->binBuildNumber >> 24) & 0xFF);
	printf("%-30s : %d\n",
	       "Cal Bin Minor Ver",
	       (pBase->binBuildNumber >> 16) & 0xFF);
	printf("%-30s : %d\n",
	       "Cal Bin Build",
	       (pBase->binBuildNumber >> 8) & 0xFF);

	if (eep_9285_get_rev(emp) >= AR5416_EEP_MINOR_VER_3) {
		printf("%-30s : %s\n",
		       "Device Type",
		       sDeviceType[(pBase->deviceType & 0x7)]);
	}

	printf("\nCustomer Data in hex:\n");
	for (i = 0; i < ARRAY_SIZE(eep->custData); i++) {
		printf("%02X ", eep->custData[i]);
		if ((i % 16) == 15)
			printf("\n");
	}

	printf("\n");
}

static void eep_9285_dump_modal_header(struct edump *edump)
{
#define PR(_token, _p, _val_fmt, _val)			\
	do {						\
		printf("%-23s %-2s", (_token), ":");	\
		printf("%s%"_val_fmt, _p, (_val));	\
		printf("\n");				\
	} while(0)

	struct eep_9285_priv *emp = edump->eepmap_priv;
	struct ar9285_eeprom *eep = &emp->eep;
	struct ar9285_modal_eep_hdr *pModal = &eep->modalHeader;

	EEP_PRINT_SECT_NAME("EEPROM Modal Header");

	PR("Ant Chain 0", "0x", "X", pModal->antCtrlChain[0]);
	PR("Antenna Common", "0x", "X", pModal->antCtrlCommon);
	PR("Antenna Gain Chain 0", "", "d", pModal->antennaGainCh[0]);
	PR("Switch Settling", "", "d", pModal->switchSettling);
	PR("TxRxAttenation Chain 0", "", "d", pModal->txRxAttenCh[0]);
	PR("RxTxMargin Chain 0", "", "d", pModal->rxTxMarginCh[0]);
	PR("ADC Desired Size", "", "d", pModal->adcDesiredSize);
	PR("PGA Desired Size", "", "d", pModal->pgaDesiredSize);
	PR("XLNA Gain Chain 0", "", "d", pModal->xlnaGainCh[0]);
	PR("TxEndToXpaOff", "", "d", pModal->txEndToXpaOff);
	PR("TxEndToRxOn", "", "d", pModal->txEndToRxOn);
	PR("TxFrameToXpaOn", "", "d", pModal->txFrameToXpaOn);
	PR("Thresh 62", "", "d", pModal->thresh62);
	PR("NF Thresh Chain 0", "", "d", pModal->noiseFloorThreshCh[0]);
	PR("XPD Gain", "", "d", pModal->xpdGain);
	PR("XPD", "", "d", pModal->xpd);
	PR("IQ Cal I Chain 0", "", "d", pModal->iqCalICh[0]);
	PR("IQ Cal Q Chain 0", "", "d", pModal->iqCalQCh[0]);
	PR("PD Gain Overlap", "", "d", pModal->pdGainOverlap);
	PR("Output Bias CCK", "", "d", pModal->ob_0);
	PR("Output Bias BPSK", "", "d", pModal->ob_1);
	PR("Driver 1 Bias CCK", "", "d", pModal->db1_0);
	PR("Driver 1 Bias BPSK", "", "d", pModal->db1_1);
	PR("XPA Bias Level", "", "d", pModal->xpaBiasLvl);
	PR("TX Frame to Data Start", "", "d", pModal->txFrameToDataStart);
	PR("TX Frame to PA On", "", "d", pModal->txFrameToPaOn);
	PR("HT40PowerIncForPDADC", "", "d", pModal->ht40PowerIncForPdadc);
	PR("bsw_atten Chain 0", "", "d", pModal->bswAtten[0]);
	PR("bsw_margin Chain 0", "", "d", pModal->bswMargin[0]);
	PR("Switch Settling [HT40]", "", "d", pModal->swSettleHt40);
	PR("xatten2DB Chain 0", "", "d", pModal->xatten2Db[0]);
	PR("xatten2margin Chain 0", "", "d", pModal->xatten2Margin[0]);
	PR("Driver 2 Bias CCK", "", "d", pModal->db2_0);
	PR("Driver 2 Bias BPSK", "", "d", pModal->db2_1);
	PR("ob_db Version", "", "d", pModal->version);
	PR("Output Bias QPSK", "", "d", pModal->ob_2);
	PR("Output Bias 16QAM", "", "d", pModal->ob_3);
	PR("Output Bias 64QAM", "", "d", pModal->ob_4);
	PR("Ant diversity ctrl 1", "", "d", pModal->antdiv_ctl1);
	PR("Driver 1 Bias QPSK", "", "d", pModal->db1_2);
	PR("Driver 1 Bias 16QAM", "", "d", pModal->db1_3);
	PR("Driver 1 Bias 64QAM", "", "d", pModal->db1_4);
	PR("Ant diversity ctrl 2", "", "d", pModal->antdiv_ctl2);
	PR("Driver 2 Bias QPSK", "", "d", pModal->db2_2);
	PR("Driver 2 Bias 16QAM", "", "d", pModal->db2_3);
	PR("Driver 2 Bias 64QAM", "", "d", pModal->db2_4);

	printf("\n");
}

static void eep_9285_dump_power_info(struct edump *edump)
{
#define PR_TARGET_POWER(__pref, __field, __rates)			\
		EEP_PRINT_SUBSECT_NAME(__pref " per-rate target power");\
		ar5416_dump_target_power((void *)eep->__field,		\
				 ARRAY_SIZE(eep->__field),		\
				 __rates, ARRAY_SIZE(__rates), 1);	\
		printf("\n");

	struct eep_9285_priv *emp = edump->eepmap_priv;
	const struct ar9285_eeprom *eep = &emp->eep;

	EEP_PRINT_SECT_NAME("EEPROM Power Info");

	PR_TARGET_POWER("2 GHz CCK", calTargetPowerCck, eep_rates_cck);
	PR_TARGET_POWER("2 GHz OFDM", calTargetPower2G, eep_rates_ofdm);
	PR_TARGET_POWER("2 GHz HT20", calTargetPower2GHT20, eep_rates_ht);
	PR_TARGET_POWER("2 GHz HT40", calTargetPower2GHT40, eep_rates_ht);

	EEP_PRINT_SUBSECT_NAME("CTL data");
	ar5416_dump_ctl(eep->ctlIndex, &eep->ctlData[0].ctlEdges[0][0],
			AR9285_NUM_CTLS, AR9285_MAX_CHAINS, 1,
			AR9285_NUM_BAND_EDGES);

#undef PR_TARGET_POWER
}

const struct eepmap eepmap_9285 = {
	.name = "9285",
	.desc = "AR9285 chip EEPROM map",
	.priv_data_sz = sizeof(struct eep_9285_priv),
	.eep_buf_sz = AR9285_DATA_START_LOC + AR9285_DATA_SZ,
	.fill_eeprom  = eep_9285_fill,
	.check_eeprom = eep_9285_check,
	.dump = {
		[EEP_SECT_INIT] = eep_9285_dump_init_data,
		[EEP_SECT_BASE] = eep_9285_dump_base_header,
		[EEP_SECT_MODAL] = eep_9285_dump_modal_header,
		[EEP_SECT_POWER] = eep_9285_dump_power_info,
	},
};
