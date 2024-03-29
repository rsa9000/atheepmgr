/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * Copyright (c) 2013,2016-2020 Sergey Ryazanov <ryazanov.s.a@gmail.com>
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
#include "eep_9285.h"

struct eep_9285_priv {
	union {
		struct ar5416_eep_init ini;
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

static bool eep_9285_load_eeprom(struct atheepmgr *aem, bool raw)
{
	struct eep_9285_priv *emp = aem->eepmap_priv;
	uint16_t *eep_data = (uint16_t *)&emp->eep;
	uint16_t *eep_init = (uint16_t *)&emp->ini;
	uint16_t *buf = aem->eep_buf;
	int addr;

	/* Check byteswaping requirements for non-RAW operation */
	if (!raw && !AR5416_TOGGLE_BYTESWAP(9285))
		return false;

	/* Read to the intermediate buffer */
	for (addr = 0; addr < AR9285_DATA_START_LOC + AR9285_DATA_SZ; ++addr) {
		if (!EEP_READ(addr, &buf[addr])) {
			fprintf(stderr, "Unable to read EEPROM to buffer\n");
			return false;
		}
	}
	aem->eep_len = addr;

	if (raw)	/* Earlier exit on RAW contents loading */
		return true;

	/* Copy from buffer to the Init data */
	for (addr = 0; addr < AR9285_DATA_START_LOC; ++addr)
		eep_init[addr] = buf[addr];

	/* Copy from buffer to the EEPROM structure */
	for (addr = 0; addr < AR9285_DATA_SZ; ++addr)
		eep_data[addr] = buf[AR9285_DATA_START_LOC + addr];

	return true;
}

static bool eep_9285_check(struct atheepmgr *aem)
{
	struct eep_9285_priv *emp = aem->eepmap_priv;
	struct ar5416_eep_init *ini = &emp->ini;
	struct ar9285_eeprom *eep = &emp->eep;
	struct ar9285_base_eep_hdr *pBase = &eep->baseEepHeader;
	const uint16_t *buf = aem->eep_buf;
	uint16_t sum;
	int i, el;

	if (ini->magic != AR5416_EEPROM_MAGIC &&
	    bswap_16(ini->magic) != AR5416_EEPROM_MAGIC) {
		fprintf(stderr, "Invalid EEPROM Magic 0x%04x, expected 0x%04x\n",
			ini->magic, AR5416_EEPROM_MAGIC);
		return false;
	}

	if (!!(pBase->eepMisc & AR5416_EEPMISC_BIG_ENDIAN) != aem->host_is_be) {
		struct ar9285_modal_eep_hdr *pModal;

		printf("EEPROM Endianness is not native.. Changing\n");

		for (i = 0; i < ARRAY_SIZE(emp->init_data); ++i)
			bswap_16_inplace(emp->init_data[i]);

		bswap_16_inplace(pBase->length);
		bswap_16_inplace(pBase->checksum);
		bswap_16_inplace(pBase->version);
		bswap_16_inplace(pBase->regDmn[0]);
		bswap_16_inplace(pBase->regDmn[1]);
		bswap_16_inplace(pBase->rfSilent);
		bswap_16_inplace(pBase->blueToothOptions);
		bswap_16_inplace(pBase->deviceCap);
		bswap_32_inplace(pBase->binBuildNumber);

		pModal = &eep->modalHeader;
		bswap_32_inplace(pModal->antCtrlCommon);
		for (i = 0; i < ARRAY_SIZE(pModal->antCtrlChain); ++i)
			bswap_32_inplace(pModal->antCtrlChain[i]);
		for (i = 0; i < ARRAY_SIZE(pModal->spurChans); ++i)
			bswap_16_inplace(pModal->spurChans[i].spurChan);
	}

	if (eep_9285_get_ver(emp) != AR5416_EEP_VER ||
	    eep_9285_get_rev(emp) < AR5416_EEP_NO_BACK_VER) {
		fprintf(stderr, "Bad EEPROM version 0x%04x (%d.%d)\n",
			pBase->version, eep_9285_get_ver(emp),
			eep_9285_get_rev(emp));
		return false;
	}

	el = pBase->length / sizeof(uint16_t);
	if (el > AR9285_DATA_SZ)
		el = AR9285_DATA_SZ;

	sum = eep_calc_csum(&buf[AR9285_DATA_START_LOC], el);
	if (sum != 0xffff) {
		fprintf(stderr, "Bad EEPROM checksum 0x%04x\n", sum);
		return false;
	}

	return true;
}

static void eep_9285_dump_init_data(struct atheepmgr *aem)
{
	struct eep_9285_priv *emp = aem->eepmap_priv;
	struct ar5416_eep_init *ini = &emp->ini;

	EEP_PRINT_SECT_NAME("Chip init data");

	ar5416_dump_eep_init(ini, sizeof(emp->init_data) / 2);
}

static void eep_9285_dump_base_header(struct atheepmgr *aem)
{
	struct eep_9285_priv *emp = aem->eepmap_priv;
	struct ar9285_eeprom *eep = &emp->eep;
	struct ar9285_base_eep_hdr *pBase = &eep->baseEepHeader;

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
	hexdump_print(eep->custData, sizeof(eep->custData));

	printf("\n");
}

static void eep_9285_dump_modal_header(struct atheepmgr *aem)
{
#define PR(_token, _p, _val_fmt, _val)			\
	do {						\
		printf("%-23s %-2s", (_token), ":");	\
		printf("%s%"_val_fmt, _p, (_val));	\
		printf("\n");				\
	} while(0)

	struct eep_9285_priv *emp = aem->eepmap_priv;
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
	PR("XPD Gain", "0x", "x", pModal->xpdGain);
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

static void eep_9285_dump_power_info(struct atheepmgr *aem)
{
#define PR_TARGET_POWER(__pref, __field, __rates)			\
		EEP_PRINT_SUBSECT_NAME(__pref " per-rate target power");\
		ar5416_dump_target_power((void *)eep->__field,		\
				 ARRAY_SIZE(eep->__field),		\
				 __rates, ARRAY_SIZE(__rates), 1);	\
		printf("\n");

	struct eep_9285_priv *emp = aem->eepmap_priv;
	const struct ar9285_eeprom *eep = &emp->eep;

	EEP_PRINT_SECT_NAME("EEPROM Power Info");

	EEP_PRINT_SUBSECT_NAME("2 GHz per-freq PD cal. data");

	ar5416_dump_pwrctl_closeloop(eep->calFreqPier2G,
				     ARRAY_SIZE(eep->calFreqPier2G), 1,
				     AR9285_MAX_CHAINS,
				     eep->baseEepHeader.txMask,
				     (uint8_t *)eep->calPierData2G,
				     AR5416_PD_GAIN_ICEPTS,
				     AR9285_NUM_PD_GAINS,
				     eep->modalHeader.xpdGain,
				     AR5416_PWR_TABLE_OFFSET_DB);

	printf("\n");

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
	.features = EEPMAP_F_RAW_EEP,
	.chip_regs = {
		.srev = 0x4020,
	},
	.priv_data_sz = sizeof(struct eep_9285_priv),
	.eep_buf_sz = AR9285_DATA_START_LOC + AR9285_DATA_SZ,
	.load_eeprom  = eep_9285_load_eeprom,
	.check_eeprom = eep_9285_check,
	.dump = {
		[EEP_SECT_INIT] = eep_9285_dump_init_data,
		[EEP_SECT_BASE] = eep_9285_dump_base_header,
		[EEP_SECT_MODAL] = eep_9285_dump_modal_header,
		[EEP_SECT_POWER] = eep_9285_dump_power_info,
	},
};
