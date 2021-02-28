/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * Copyright (c) 2013,2017-2020 Sergey Ryazanov <ryazanov.s.a@gmail.com>
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
#include "eep_9287.h"

struct eep_9287_priv {
	union {
		struct ar5416_eep_init ini;
		uint16_t init_data[AR9287_DATA_START_LOC];
	};
	struct ar9287_eeprom eep;
};

static int eep_9287_get_ver(struct eep_9287_priv *emp)
{
	return (emp->eep.baseEepHeader.version >> 12) & 0xF;
}

static int eep_9287_get_rev(struct eep_9287_priv *emp)
{
	return (emp->eep.baseEepHeader.version) & 0xFFF;
}

static bool eep_9287_load_eeprom(struct atheepmgr *aem)
{
	struct eep_9287_priv *emp = aem->eepmap_priv;
	uint16_t *eep_data = (uint16_t *)&emp->eep;
	uint16_t *eep_init = (uint16_t *)&emp->ini;
	uint16_t *buf = aem->eep_buf;
	int addr;

	/* Check byteswaping requirements */
	if (!AR5416_TOGGLE_BYTESWAP(9287))
		return false;

	/* Read to the intermediate buffer */
	for (addr = 0; addr < AR9287_DATA_START_LOC + AR9287_DATA_SZ; ++addr) {
		if (!EEP_READ(addr, &buf[addr])) {
			fprintf(stderr, "Unable to read EEPROM to buffer\n");
			return false;
		}
	}
	aem->eep_len = addr;

	/* Copy from buffer to the Init data */
	for (addr = 0; addr < AR9287_DATA_START_LOC; ++addr)
		eep_init[addr] = buf[addr];

	/* Copy from buffer to the EEPROM structure */
	for (addr = 0; addr < AR9287_DATA_SZ; ++addr)
		eep_data[addr] = buf[AR9287_DATA_START_LOC + addr];

	return true;
}

static bool eep_9287_check_eeprom(struct atheepmgr *aem)
{
	struct eep_9287_priv *emp = aem->eepmap_priv;
	struct ar5416_eep_init *ini = &emp->ini;
	struct ar9287_eeprom *eep = &emp->eep;
	struct ar9287_base_eep_hdr *pBase = &eep->baseEepHeader;
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
		struct ar9287_modal_eep_hdr *pModal;

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

	if (eep_9287_get_ver(emp) != AR5416_EEP_VER ||
	    eep_9287_get_rev(emp) < AR5416_EEP_NO_BACK_VER) {
		fprintf(stderr, "Bad EEPROM version 0x%04x (%d.%d)\n",
			pBase->version, eep_9287_get_ver(emp),
			eep_9287_get_rev(emp));
		return false;
	}

	el = pBase->length / sizeof(uint16_t);
	if (el > AR9287_DATA_SZ)
		el = AR9287_DATA_SZ;

	sum = eep_calc_csum(&buf[AR9287_DATA_START_LOC], el);
	if (sum != 0xffff) {
		fprintf(stderr, "Bad EEPROM checksum 0x%04x\n", sum);
		return false;
	}

	return true;
}

static void eep_9287_dump_init_data(struct atheepmgr *aem)
{
	struct eep_9287_priv *emp = aem->eepmap_priv;
	struct ar5416_eep_init *ini = &emp->ini;

	EEP_PRINT_SECT_NAME("Chip init data");

	ar5416_dump_eep_init(ini, sizeof(emp->init_data) / 2);
}

static void eep_9287_dump_base_header(struct atheepmgr *aem)
{
	struct eep_9287_priv *emp = aem->eepmap_priv;
	struct ar9287_eeprom *eep = &emp->eep;
	struct ar9287_base_eep_hdr *pBase = &eep->baseEepHeader;

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
	       "Wake on Wireless",
	       !!(pBase->eepMisc & AR9287_EEPMISC_WOW));
	printf("%-30s : %d\n",
	       "Cal Bin Major Ver",
	       (pBase->binBuildNumber >> 24) & 0xFF);
	printf("%-30s : %d\n",
	       "Cal Bin Minor Ver",
	       (pBase->binBuildNumber >> 16) & 0xFF);
	printf("%-30s : %d\n",
	       "Cal Bin Build",
	       (pBase->binBuildNumber >> 8) & 0xFF);
	printf("%-30s : %d\n",
	       "OpenLoop PowerControl",
	       (pBase->openLoopPwrCntl & 0x1));
	printf("%-30s : %d\n", "Power Table Offset, dBm",
	       pBase->pwrTableOffset);

	if (eep_9287_get_rev(emp) >= AR5416_EEP_MINOR_VER_3) {
		printf("%-30s : %s\n",
		       "Device Type",
		       sDeviceType[(pBase->deviceType & 0x7)]);
	}

	printf("\nCustomer Data in hex:\n");
	hexdump_print(eep->custData, sizeof(eep->custData));

	printf("\n");
}

static void eep_9287_dump_modal_header(struct atheepmgr *aem)
{
#define PR(_token, _p, _val_fmt, _val)			\
	do {						\
		printf("%-23s %-2s", (_token), ":");	\
		printf("%s%"_val_fmt, _p, (_val));	\
		printf("\n");				\
	} while(0)

	struct eep_9287_priv *emp = aem->eepmap_priv;
	struct ar9287_eeprom *eep = &emp->eep;
	struct ar9287_modal_eep_hdr *pModal = &eep->modalHeader;

	EEP_PRINT_SECT_NAME("EEPROM Modal Header");

	PR("Chain0 Ant. Control", "0x", "X", pModal->antCtrlChain[0]);
	PR("Chain1 Ant. Control", "0x", "X", pModal->antCtrlChain[1]);
	PR("Ant. Common Control", "0x", "X", pModal->antCtrlCommon);
	PR("Chain0 Ant. Gain", "", "d", pModal->antennaGainCh[0]);
	PR("Chain1 Ant. Gain", "", "d", pModal->antennaGainCh[1]);
	PR("Switch Settle", "", "d", pModal->switchSettling);
	PR("Chain0 TxRxAtten", "", "d", pModal->txRxAttenCh[0]);
	PR("Chain1 TxRxAtten", "", "d", pModal->txRxAttenCh[1]);
	PR("Chain0 RxTxMargin", "", "d", pModal->rxTxMarginCh[0]);
	PR("Chain1 RxTxMargin", "", "d", pModal->rxTxMarginCh[1]);
	PR("ADC Desired size", "", "d", pModal->adcDesiredSize);
	PR("txEndToXpaOff", "", "d", pModal->txEndToXpaOff);
	PR("txEndToRxOn", "", "d", pModal->txEndToRxOn);
	PR("txFrameToXpaOn", "", "d", pModal->txFrameToXpaOn);
	PR("CCA Threshold)", "", "d", pModal->thresh62);
	PR("Chain0 NF Threshold", "", "d", pModal->noiseFloorThreshCh[0]);
	PR("Chain1 NF Threshold", "", "d", pModal->noiseFloorThreshCh[1]);
	PR("xpdGain", "0x", "x", pModal->xpdGain);
	PR("External PD", "", "d", pModal->xpd);
	PR("Chain0 I Coefficient", "", "d", pModal->iqCalICh[0]);
	PR("Chain1 I Coefficient", "", "d", pModal->iqCalICh[1]);
	PR("Chain0 Q Coefficient", "", "d", pModal->iqCalQCh[0]);
	PR("Chain1 Q Coefficient", "", "d", pModal->iqCalQCh[1]);
	PR("pdGainOverlap", "", "d", pModal->pdGainOverlap);
	PR("xPA Bias Level", "", "d", pModal->xpaBiasLvl);
	PR("txFrameToDataStart", "", "d", pModal->txFrameToDataStart);
	PR("txFrameToPaOn", "", "d", pModal->txFrameToPaOn);
	PR("HT40 Power Inc.", "", "d", pModal->ht40PowerIncForPdadc);
	PR("Chain0 bswAtten", "", "d", pModal->bswAtten[0]);
	PR("Chain1 bswAtten", "", "d", pModal->bswAtten[1]);
	PR("Chain0 bswMargin", "", "d", pModal->bswMargin[0]);
	PR("Chain1 bswMargin", "", "d", pModal->bswMargin[1]);
	PR("HT40 Switch Settle", "", "d", pModal->swSettleHt40);
	PR("AR92x7 Version", "", "d", pModal->version);
	PR("DriverBias1", "", "d", pModal->db1);
	PR("DriverBias2", "", "d", pModal->db1);
	PR("CCK OutputBias", "", "d", pModal->ob_cck);
	PR("PSK OutputBias", "", "d", pModal->ob_psk);
	PR("QAM OutputBias", "", "d", pModal->ob_qam);
	PR("PAL_OFF OutputBias", "", "d", pModal->ob_pal_off);

	printf("\n");
}

static void
eep_9287_dump_pwrctl_openloop_item(const struct ar9287_cal_data_op_loop *data,
				   int gainmask)
{
	const char * const gains[AR5416_NUM_PD_GAINS] = {"4", "2", "1", "0.5"};
	int i, pos;

	printf("          Field: pwrPdg vpdPdg  pcdac  empty\n");
	printf("      ---------- ------ ------ ------ ------\n");
	pos = -1;
	for (i = 0; i < ARRAY_SIZE(gains); ++i) {
		if (!(gainmask & (1 << i)))
			continue;
		pos++;
		if (pos >= ARRAY_SIZE(data->pwrPdg)) {
			printf("      Too many gains activated, no data available\n");
			break;
		}
		printf("      Gain x%-3s:", gains[i]);

		/**
		 * In all dumps what I saw, only the first elements of arrays
		 * are filled with meaningful data. Moreover driver uses only
		 * first elements too. So do not even try to output garbage
		 * from other elements.
		 */
		printf("  %5.2f", (double)data->pwrPdg[pos][0] / 4);
		printf("  %5u", data->vpdPdg[pos][0]);
		printf("  %5u", data->pcdac[pos][0]);
		printf("  %5u", data->empty[pos][0]);
		printf("\n");
	}
}

static void
eep_9287_dump_pwrctl_openloop(const uint8_t *freqs, int chainmask, int gainmask,
			      const union ar9287_cal_data_per_freq_u *data,
			      int power_table_offset)
{
	const union ar9287_cal_data_per_freq_u *item;
	int chain, freq;	/* Indexes */

	for (chain = 0; chain < AR9287_MAX_CHAINS; ++chain) {
		if (!(chainmask & (1 << chain)))
			continue;
		printf("  Chain %d:\n", chain);
		printf("\n");
		for (freq = 0; freq < AR9287_NUM_2G_CAL_PIERS; ++freq) {
			if (freqs[freq] == AR5416_BCHAN_UNUSED)
				break;

			printf("    %4u MHz:\n", FBIN2FREQ(freqs[freq], 1));
			item = data + (chain * AR9287_NUM_2G_CAL_PIERS + freq);

			eep_9287_dump_pwrctl_openloop_item(&item->calDataOpen,
							   gainmask);

			printf("\n");
		}
	}
}

static void eep_9287_dump_power_info(struct atheepmgr *aem)
{
#define PR_TARGET_POWER(__pref, __field, __rates)			\
		EEP_PRINT_SUBSECT_NAME(__pref " per-rate target power");\
		ar5416_dump_target_power((void *)eep->__field,		\
				 ARRAY_SIZE(eep->__field),		\
				 __rates, ARRAY_SIZE(__rates), 1);	\
		printf("\n");

	struct eep_9287_priv *emp = aem->eepmap_priv;
	const struct ar9287_eeprom *eep = &emp->eep;
	int maxradios = 0, i;

	EEP_PRINT_SECT_NAME("EEPROM Power Info");

	EEP_PRINT_SUBSECT_NAME("2 GHz per-freq PD cal. data");

	if (eep->baseEepHeader.openLoopPwrCntl & 0x01) {
		eep_9287_dump_pwrctl_openloop(eep->calFreqPier2G,
					      eep->baseEepHeader.txMask,
					      eep->modalHeader.xpdGain,
					      &eep->calPierData2G[0][0],
					      eep->baseEepHeader.pwrTableOffset);
	} else {
		ar5416_dump_pwrctl_closeloop(eep->calFreqPier2G,
					     ARRAY_SIZE(eep->calFreqPier2G), 1,
					     AR9287_MAX_CHAINS,
					     eep->baseEepHeader.txMask,
					     (uint8_t *)eep->calPierData2G,
					     AR9287_PD_GAIN_ICEPTS,
					     AR5416_NUM_PD_GAINS,
					     eep->modalHeader.xpdGain,
					     eep->baseEepHeader.pwrTableOffset);
	}

	printf("\n");

	PR_TARGET_POWER("2 GHz CCK", calTargetPowerCck, eep_rates_cck);
	PR_TARGET_POWER("2 GHz OFDM", calTargetPower2G, eep_rates_ofdm);
	PR_TARGET_POWER("2 GHz HT20", calTargetPower2GHT20, eep_rates_ht);
	PR_TARGET_POWER("2 GHz HT40", calTargetPower2GHT40, eep_rates_ht);

	EEP_PRINT_SUBSECT_NAME("CTL data");
	for (i = 0; i < AR9287_MAX_CHAINS; ++i) {
		if (eep->baseEepHeader.txMask & (1 << i))
			maxradios++;
	}
	ar5416_dump_ctl(eep->ctlIndex, &eep->ctlData[0].ctlEdges[0][0],
			AR9287_NUM_CTLS, AR9287_MAX_CHAINS, maxradios,
			AR9287_NUM_BAND_EDGES);

#undef PR_TARGET_POWER
}

const struct eepmap eepmap_9287 = {
	.name = "9287",
	.desc = "AR9287 chip EEPROM map",
	.chip_regs = {
		.srev = 0x4020,
	},
	.priv_data_sz = sizeof(struct eep_9287_priv),
	.eep_buf_sz = AR9287_DATA_START_LOC + AR9287_DATA_SZ,
	.load_eeprom  = eep_9287_load_eeprom,
	.check_eeprom = eep_9287_check_eeprom,
	.dump = {
		[EEP_SECT_INIT] = eep_9287_dump_init_data,
		[EEP_SECT_BASE] = eep_9287_dump_base_header,
		[EEP_SECT_MODAL] = eep_9287_dump_modal_header,
		[EEP_SECT_POWER] = eep_9287_dump_power_info,
	},
};
