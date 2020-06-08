/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * Copyright (c) 2013,2017-2019 Sergey Ryazanov <ryazanov.s.a@gmail.com>
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
#include "eep_5416.h"

struct eep_5416_priv {
	union {
		struct ar5416_init ini;
		uint16_t init_data[AR5416_DATA_START_LOC];
	};
	struct ar5416_eeprom eep;
};

static int eep_5416_get_ver(struct eep_5416_priv *emp)
{
	return ((emp->eep.baseEepHeader.version >> 12) & 0xF);
}

static int eep_5416_get_rev(struct eep_5416_priv *emp)
{
	return ((emp->eep.baseEepHeader.version) & 0xFFF);
}

static bool eep_5416_fill(struct atheepmgr *aem)
{
	struct eep_5416_priv *emp = aem->eepmap_priv;
	uint16_t *eep_data = (uint16_t *)&emp->eep;
	uint16_t *eep_init = (uint16_t *)&emp->ini;
	uint16_t *buf = aem->eep_buf;
	uint16_t magic;
	int addr;

	/* Check byteswaping requirements */
	if (!EEP_READ(AR5416_EEPROM_MAGIC_OFFSET, &magic)) {
		fprintf(stderr, "EEPROM magic read failed\n");
		return false;
	}
	if (bswap_16(magic) == AR5416_EEPROM_MAGIC)
		aem->eep_io_swap = !aem->eep_io_swap;

	/* Read to the intermediate buffer */
	for (addr = 0; addr < AR5416_DATA_START_LOC + AR5416_DATA_SZ; ++addr) {
		if (!EEP_READ(addr, &buf[addr])) {
			fprintf(stderr, "Unable to read EEPROM to buffer\n");
			return false;
		}
	}
	aem->eep_len = addr;

	/* Copy from buffer to the Init data */
	for (addr = 0; addr < AR5416_DATA_START_LOC; ++addr)
		eep_init[addr] = buf[addr];

	/* Copy from buffer to the EEPROM structure */
	for (addr = 0; addr < AR5416_DATA_SZ; ++addr)
		eep_data[addr] = buf[AR5416_DATA_START_LOC + addr];

	return true;
}

static bool eep_5416_check(struct atheepmgr *aem)
{
	struct eep_5416_priv *emp = aem->eepmap_priv;
	struct ar5416_init *ini = &emp->ini;
	struct ar5416_eeprom *eep = &emp->eep;
	const uint16_t *buf = aem->eep_buf;
	uint16_t sum;
	int i, el;

	if (ini->magic != AR5416_EEPROM_MAGIC) {
		fprintf(stderr, "Invalid EEPROM Magic 0x%04x, expected 0x%04x\n",
			ini->magic, AR5416_EEPROM_MAGIC);
		return false;
	}

	if (!!(eep->baseEepHeader.eepMisc & AR5416_EEPMISC_BIG_ENDIAN) !=
	    aem->host_is_be) {
		uint32_t integer;
		uint16_t word;

		printf("EEPROM Endianness is not native.. Changing.\n");

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

		integer = bswap_32(eep->baseEepHeader.binBuildNumber);
		eep->baseEepHeader.binBuildNumber = integer;

		integer = bswap_32(eep->modalHeader5G.antCtrlCommon);
		eep->modalHeader5G.antCtrlCommon = integer;

		for (i = 0; i < AR5416_MAX_CHAINS; i++) {
			integer = bswap_32(eep->modalHeader5G.antCtrlChain[i]);
			eep->modalHeader5G.antCtrlChain[i] = integer;
		}

		for (i = 0; i < AR_EEPROM_MODAL_SPURS; i++) {
			word = bswap_16(eep->modalHeader5G.spurChans[i].spurChan);
			eep->modalHeader5G.spurChans[i].spurChan = word;
		}

		integer = bswap_32(eep->modalHeader2G.antCtrlCommon);
		eep->modalHeader2G.antCtrlCommon = integer;

		for (i = 0; i < AR5416_MAX_CHAINS; i++) {
			integer = bswap_32(eep->modalHeader2G.antCtrlChain[i]);
			eep->modalHeader2G.antCtrlChain[i] = integer;
		}

		for (i = 0; i < AR_EEPROM_MODAL_SPURS; i++) {
			word = bswap_16(eep->modalHeader2G.spurChans[i].spurChan);
			eep->modalHeader2G.spurChans[i].spurChan = word;
		}
	}

	if (eep_5416_get_ver(emp) != AR5416_EEP_VER ||
	    eep_5416_get_rev(emp) < AR5416_EEP_NO_BACK_VER) {
		fprintf(stderr, "Bad EEPROM version 0x%04x (%d.%d)\n",
			eep->baseEepHeader.version, eep_5416_get_ver(emp),
			eep_5416_get_rev(emp));
		return false;
	}

	el = eep->baseEepHeader.length / sizeof(uint16_t);
	if (el > AR5416_DATA_SZ)
		el = AR5416_DATA_SZ;

	sum = eep_calc_csum(&buf[AR5416_DATA_START_LOC], el);
	if (sum != 0xffff) {
		fprintf(stderr, "Bad EEPROM checksum 0x%04x\n", sum);
		return false;
	}

	return true;
}

static void eep_5416_dump_init_data(struct atheepmgr *aem)
{
	struct eep_5416_priv *emp = aem->eepmap_priv;
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

static void eep_5416_dump_base_header(struct atheepmgr *aem)
{
	struct eep_5416_priv *emp = aem->eepmap_priv;
	struct ar5416_eeprom *ar5416Eep = &emp->eep;
	struct ar5416_base_eep_hdr *pBase = &ar5416Eep->baseEepHeader;

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
	if (eep_5416_get_rev(emp) >= AR5416_EEP_MINOR_VER_19) {
		printf("%-30s : %s\n",
		       "OpenLoopPwrCntl",
		       pBase->openLoopPwrCntl ? "true" : "false");
	}
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
	printf("%-30s : %-4.1f\n",
	       "Power table offset",
	       (double)pBase->power_table_offset / 2);

	if (eep_5416_get_rev(emp) >= AR5416_EEP_MINOR_VER_3) {
		printf("%-30s : %s\n",
		       "Device Type",
		       sDeviceType[(pBase->deviceType & 0x7)]);
	}

	printf("\nCustomer Data in hex:\n");
	hexdump_print(ar5416Eep->custData, sizeof(ar5416Eep->custData));

	printf("\n");
}

static void eep_5416_dump_modal_header(struct atheepmgr *aem)
{
#define _PR(_token, _fmt, _field)				\
	do {							\
		printf("%-23s :", _token);			\
		if (pBase->opCapFlags & AR5416_OPFLAGS_11G) {	\
			snprintf(buf, sizeof(buf), _fmt,	\
				 ar5416Eep->modalHeader2G._field);\
			printf("%7s%-7s", "", buf);		\
		}						\
		if (pBase->opCapFlags & AR5416_OPFLAGS_11A) {	\
			snprintf(buf, sizeof(buf), _fmt,	\
				 ar5416Eep->modalHeader5G._field);\
			printf("%7s%s", "", buf);		\
		}						\
		printf("\n");					\
	} while(0)
#define PR_DEC(_token, _field)					\
		_PR(_token, "%d", _field)
#define PR_HEX(_token, _field)					\
		_PR(_token, "0x%X", _field)
#define PR_PWR(_token, _field)					\
		_PR(_token, "%.1f", _field / (double)2.0);

	struct eep_5416_priv *emp = aem->eepmap_priv;
	struct ar5416_eeprom *ar5416Eep = &emp->eep;
	struct ar5416_base_eep_hdr *pBase = &ar5416Eep->baseEepHeader;
	char buf[0x10];

	EEP_PRINT_SECT_NAME("EEPROM Modal Header");

	printf("%20s", "");
	if (pBase->opCapFlags & AR5416_OPFLAGS_11G)
		printf("%14s", "2G");
	if (pBase->opCapFlags & AR5416_OPFLAGS_11A)
		printf("%14s", "5G");
	printf("\n\n");

	PR_HEX("Ant Chain 0", antCtrlChain[0]);
	PR_HEX("Ant Chain 1", antCtrlChain[1]);
	PR_HEX("Ant Chain 2", antCtrlChain[2]);
	PR_HEX("Antenna Common", antCtrlCommon);
	PR_DEC("Antenna Gain Chain 0", antennaGainCh[0]);
	PR_DEC("Antenna Gain Chain 1", antennaGainCh[1]);
	PR_DEC("Antenna Gain Chain 2", antennaGainCh[2]);
	PR_DEC("Switch Settling", switchSettling);
	PR_DEC("TxRxAttenuation Ch 0", txRxAttenCh[0]);
	PR_DEC("TxRxAttenuation Ch 1", txRxAttenCh[1]);
	PR_DEC("TxRxAttenuation Ch 2", txRxAttenCh[2]);
	PR_DEC("RxTxMargin Chain 0", rxTxMarginCh[0]);
	PR_DEC("RxTxMargin Chain 1", rxTxMarginCh[1]);
	PR_DEC("RxTxMargin Chain 2", rxTxMarginCh[2]);
	PR_DEC("ADC Desired Size", adcDesiredSize);
	PR_DEC("PGA Desired Size", pgaDesiredSize);
	PR_DEC("TX end to xlna on", txEndToRxOn);
	PR_DEC("xlna gain Chain 0", xlnaGainCh[0]);
	PR_DEC("xlna gain Chain 1", xlnaGainCh[1]);
	PR_DEC("xlna gain Chain 2", xlnaGainCh[2]);
	PR_DEC("TX end to xpa off", txEndToXpaOff);
	PR_DEC("TX frame to xpa on", txFrameToXpaOn);
	PR_DEC("THRESH62", thresh62);
	PR_DEC("NF Thresh 0", noiseFloorThreshCh[0]);
	PR_DEC("NF Thresh 1", noiseFloorThreshCh[1]);
	PR_DEC("NF Thresh 2", noiseFloorThreshCh[2]);
	PR_HEX("Xpd Gain Mask", xpdGain);
	PR_DEC("Xpd", xpd);
	PR_DEC("IQ Cal I Chain 0", iqCalICh[0]);
	PR_DEC("IQ Cal I Chain 1", iqCalICh[1]);
	PR_DEC("IQ Cal I Chain 2", iqCalICh[2]);
	PR_DEC("IQ Cal Q Chain 0", iqCalQCh[0]);
	PR_DEC("IQ Cal Q Chain 1", iqCalQCh[1]);
	PR_DEC("IQ Cal Q Chain 2", iqCalQCh[2]);
	PR_DEC("Analog Output Bias(ob)", ob);
	PR_DEC("Analog Driver Bias(db)", db);
	PR_DEC("Xpa bias level", xpaBiasLvl);
	PR_DEC("Xpa bias level Freq 0", xpaBiasLvlFreq[0]);
	PR_DEC("Xpa bias level Freq 1", xpaBiasLvlFreq[1]);
	PR_DEC("Xpa bias level Freq 2", xpaBiasLvlFreq[2]);
	PR_HEX("LNA Control", lna_ctl);

	PR_PWR("pdGain Overlap (dB)", pdGainOverlap);
	PR_PWR("PWR dec 2 chain", pwrDecreaseFor2Chain);
	PR_PWR("PWR dec 3 chain", pwrDecreaseFor3Chain);

	if (AR_SREV_9280_20_OR_LATER(aem)) {
		PR_DEC("xatten2Db Chain 0", xatten2Db[0]);
		PR_DEC("xatten2Db Chain 1", xatten2Db[1]);
		PR_DEC("xatten2Margin Chain 0", xatten2Margin[0]);
		PR_DEC("xatten2Margin Chain 1", xatten2Margin[1]);
		PR_DEC("ob_ch1", ob_ch1);
		PR_DEC("db_ch1", db_ch1);
	}

	if (eep_5416_get_rev(emp) >= AR5416_EEP_MINOR_VER_3) {
		PR_DEC("txFrameToDataStart", txFrameToDataStart);
		PR_DEC("txFrameToPaOn", txFrameToPaOn);
		PR_DEC("HT40PowerIncForPDADC", ht40PowerIncForPdadc);
		PR_DEC("bswAtten Chain 0", bswAtten[0]);
	}

	printf("\n");

#undef PR_PWR
#undef PR_HEX
#undef PR_DEC
#undef _PR
}

static void
eep_5416_dump_closeloop_item(const struct ar5416_cal_data_per_freq *item,
			     int gainmask)
{
	const char * const gains[AR5416_NUM_PD_GAINS] = {"0.5", "1", "2", "4"};
	struct {
		uint8_t pwr;
		uint8_t vpd[AR5416_NUM_PD_GAINS];
	} merged[AR5416_PD_GAIN_ICEPTS * AR5416_NUM_PD_GAINS];
	int gii[AR5416_NUM_PD_GAINS];	/* Array of indexes for merge */
	int gain, pwr, npwr;		/* Indexes */
	uint8_t pwrmin;

	/* Merge calibration per-gain power lists to filter duplicates */
	memset(merged, 0xff, sizeof(merged));
	memset(gii, 0x00, sizeof(gii));
	for (pwr = 0; pwr < ARRAY_SIZE(merged); ++pwr) {
		pwrmin = 0xff;
		for (gain = 0; gain < ARRAY_SIZE(gii); ++gain) {
			if (!(gainmask & (1 << gain)) ||
			    gii[gain] >= AR5416_PD_GAIN_ICEPTS)
				continue;
			if (item->pwrPdg[gain][gii[gain]] < pwrmin)
				pwrmin = item->pwrPdg[gain][gii[gain]];
		}
		if (pwrmin == 0xff)
			break;
		merged[pwr].pwr = pwrmin;
		for (gain = 0; gain < ARRAY_SIZE(gii); ++gain) {
			if (!(gainmask & (1 << gain)) ||
			    gii[gain] >= AR5416_PD_GAIN_ICEPTS ||
			    item->pwrPdg[gain][gii[gain]] != pwrmin)
				continue;
			merged[pwr].vpd[gain] = item->vpdPdg[gain][gii[gain]];
			gii[gain]++;
		}
	}
	npwr = pwr;

	/* Print merged data */
	printf("      Tx Power, dBm:");
	for (pwr = 0; pwr < npwr; ++pwr)
		printf(" %5.2f", (double)merged[pwr].pwr / 4);
	printf("\n");
	printf("      --------------");
	for (pwr = 0; pwr < npwr; ++pwr)
		printf(" -----");
	printf("\n");
	for (gain = 0; gain < AR5416_NUM_PD_GAINS; ++gain) {
		if (!(gainmask & (1 << gain)))
			continue;
		printf("      Gain x%-3s VPD:", gains[gain]);
		for (pwr = 0; pwr < npwr; ++pwr) {
			if (merged[pwr].vpd[gain] == 0xff)
				printf("      ");
			else
				printf("   %3u", merged[pwr].vpd[gain]);
		}
		printf("\n");
	}
}

static void eep_5416_dump_closeloop(const uint8_t *freqs, int maxfreq,
				    const struct ar5416_cal_data_per_freq *cal,
				    int is_2g, int chainmask, int gainmask)
{
	const struct ar5416_cal_data_per_freq *item;
	int chain, freq;	/* Indexes */

	for (chain = 0; chain < AR5416_MAX_CHAINS; ++chain) {
		if (!(chainmask & (1 << chain)))
			continue;
		printf("  Chain %d:\n", chain);
		printf("\n");
		for (freq = 0; freq < maxfreq; ++freq) {
			if (freqs[freq] == AR5416_BCHAN_UNUSED)
				break;

			printf("    %4u MHz:\n", FBIN2FREQ(freqs[freq], is_2g));
			item = cal + (chain * maxfreq + freq);

			eep_5416_dump_closeloop_item(item, gainmask);

			printf("\n");
		}
	}
}

static void eep_5416_dump_pd_cal(const uint8_t *freq, int maxfreq,
				 const void *caldata, int is_openloop,
				 int is_2g, int chainmask, int gainmask)
{
	if (is_openloop) {
		printf("  Open-loop PD calibration dumping is not supported\n");
	} else {
		eep_5416_dump_closeloop(freq, maxfreq, caldata, is_2g,
					chainmask, gainmask);
	}
}

static void eep_5416_dump_power_info(struct atheepmgr *aem)
{
#define PR_PD_CAL(__pref, __band, __is_2g)				\
		EEP_PRINT_SUBSECT_NAME(__pref " per-freq PD cal. data");\
		eep_5416_dump_pd_cal(eep->calFreqPier ## __band,	\
				     ARRAY_SIZE(eep->calFreqPier ## __band),\
				     eep->calPierData ## __band, is_openloop,\
				     __is_2g, eep->baseEepHeader.txMask,\
				     (eep->modalHeader ## __band).xpdGain);\
		printf("\n");
#define PR_TGT_PWR(__pref, __field, __rates, __is_2g)			\
		EEP_PRINT_SUBSECT_NAME(__pref " per-rate target power");\
		ar5416_dump_target_power((void *)eep->__field,		\
				 ARRAY_SIZE(eep->__field),		\
				 __rates, ARRAY_SIZE(__rates), __is_2g);\
		printf("\n");

	struct eep_5416_priv *emp = aem->eepmap_priv;
	const struct ar5416_eeprom *eep = &emp->eep;
	int is_openloop = 0, maxradios = 0, i;

	EEP_PRINT_SECT_NAME("EEPROM Power Info");

	if (eep_5416_get_rev(emp) >= AR5416_EEP_MINOR_VER_19 &&
	    eep->baseEepHeader.openLoopPwrCntl & 0x01)
		is_openloop = 1;

	if (eep->baseEepHeader.opCapFlags & AR5416_OPFLAGS_11G) {
		PR_PD_CAL("2 GHz", 2G, 1);
	}
	if (eep->baseEepHeader.opCapFlags & AR5416_OPFLAGS_11A) {
		PR_PD_CAL("5 GHz", 5G, 0);
	}

	if (eep->baseEepHeader.opCapFlags & AR5416_OPFLAGS_11G) {
		PR_TGT_PWR("2 GHz CCK", calTargetPowerCck, eep_rates_cck, 1);
		PR_TGT_PWR("2 GHz OFDM", calTargetPower2G, eep_rates_ofdm, 1);
		PR_TGT_PWR("2 GHz HT20", calTargetPower2GHT20, eep_rates_ht, 1);
		PR_TGT_PWR("2 GHz HT40", calTargetPower2GHT40, eep_rates_ht, 1);
	}

	if (eep->baseEepHeader.opCapFlags & AR5416_OPFLAGS_11A) {
		PR_TGT_PWR("5 GHz OFDM", calTargetPower5G, eep_rates_ofdm, 0);
		PR_TGT_PWR("5 GHz HT20", calTargetPower5GHT20, eep_rates_ht, 0);
		PR_TGT_PWR("5 GHz HT40", calTargetPower5GHT40, eep_rates_ht, 0);
	}

	EEP_PRINT_SUBSECT_NAME("CTL data");
	for (i = 0; i < AR5416_MAX_CHAINS; ++i) {
		if (eep->baseEepHeader.txMask & (1 << i))
			maxradios++;
	}
	ar5416_dump_ctl(eep->ctlIndex, &eep->ctlData[0].ctlEdges[0][0],
			AR5416_NUM_CTLS, AR5416_MAX_CHAINS, maxradios,
			AR5416_NUM_BAND_EDGES);

#undef PR_TARGET_POWER
#undef PR_PD_CAL
}

static bool eep_5416_update_eeprom(struct atheepmgr *aem, int param,
				   const void *data)
{
	struct eep_5416_priv *emp = aem->eepmap_priv;
	struct ar5416_eeprom *eep = &emp->eep;
	uint16_t *buf = aem->eep_buf;
	int data_pos, data_len = 0, addr, el;
	uint16_t sum;

	switch (param) {
	case EEP_UPDATE_MAC:
		data_pos = AR5416_DATA_START_LOC +
			   EEP_FIELD_OFFSET(baseEepHeader.macAddr);
		data_len = EEP_FIELD_SIZE(baseEepHeader.macAddr);
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

	/* Update checksum if need it */
	if (data_pos > AR5416_DATA_START_LOC) {
		el = eep->baseEepHeader.length / sizeof(uint16_t);
		if (el > AR5416_DATA_SZ)
			el = AR5416_DATA_SZ;
		buf[AR5416_DATA_CSUM_LOC] = 0xffff;
		sum = eep_calc_csum(&buf[AR5416_DATA_START_LOC], el);
		buf[AR5416_DATA_CSUM_LOC] = sum;
		if (!EEP_WRITE(AR5416_DATA_CSUM_LOC, sum)) {
			fprintf(stderr, "Unable to update EEPROM checksum\n");
			return false;
		}
	}

	return true;
}

const struct eepmap eepmap_5416 = {
	.name = "5416",
	.desc = "Default EEPROM map for earlier .11n chips (AR5416/AR9160/AR92xx/etc.)",
	.priv_data_sz = sizeof(struct eep_5416_priv),
	.eep_buf_sz = AR5416_DATA_START_LOC + AR5416_DATA_SZ,
	.fill_eeprom  = eep_5416_fill,
	.check_eeprom = eep_5416_check,
	.dump = {
		[EEP_SECT_INIT] = eep_5416_dump_init_data,
		[EEP_SECT_BASE] = eep_5416_dump_base_header,
		[EEP_SECT_MODAL] = eep_5416_dump_modal_header,
		[EEP_SECT_POWER] = eep_5416_dump_power_info,
	},
	.update_eeprom = eep_5416_update_eeprom,
	.params_mask = BIT(EEP_UPDATE_MAC),
};
