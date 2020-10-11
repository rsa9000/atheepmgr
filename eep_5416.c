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
#include "eep_5416.h"

struct eep_5416_priv {
	union {
		struct ar5416_eep_init ini;
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

static bool eep_5416_load_eeprom(struct atheepmgr *aem)
{
	struct eep_5416_priv *emp = aem->eepmap_priv;
	uint16_t *eep_data = (uint16_t *)&emp->eep;
	uint16_t *eep_init = (uint16_t *)&emp->ini;
	uint16_t *buf = aem->eep_buf;
	int addr;

	/* Check byteswaping requirements */
	if (!AR5416_TOGGLE_BYTESWAP(5416))
		return false;

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
	struct ar5416_eep_init *ini = &emp->ini;
	struct ar5416_eeprom *eep = &emp->eep;
	struct ar5416_base_eep_hdr *pBase = &eep->baseEepHeader;
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
		struct ar5416_modal_eep_hdr *pModal;

		printf("EEPROM Endianness is not native.. Changing.\n");

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

		pModal = &eep->modalHeader5G;
		bswap_32_inplace(pModal->antCtrlCommon);
		for (i = 0; i < ARRAY_SIZE(pModal->antCtrlChain); ++i)
			bswap_32_inplace(pModal->antCtrlChain[i]);
		for (i = 0; i < ARRAY_SIZE(pModal->spurChans); ++i)
			bswap_16_inplace(pModal->spurChans[i].spurChan);

		pModal = &eep->modalHeader2G;
		bswap_32_inplace(pModal->antCtrlCommon);
		for (i = 0; i < ARRAY_SIZE(pModal->antCtrlChain); ++i)
			bswap_32_inplace(pModal->antCtrlChain[i]);
		for (i = 0; i < ARRAY_SIZE(pModal->spurChans); ++i)
			bswap_16_inplace(pModal->spurChans[i].spurChan);
	}

	if (eep_5416_get_ver(emp) != AR5416_EEP_VER ||
	    eep_5416_get_rev(emp) < AR5416_EEP_NO_BACK_VER) {
		fprintf(stderr, "Bad EEPROM version 0x%04x (%d.%d)\n",
			pBase->version, eep_5416_get_ver(emp),
			eep_5416_get_rev(emp));
		return false;
	}

	el = pBase->length / sizeof(uint16_t);
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
	struct ar5416_eep_init *ini = &emp->ini;

	EEP_PRINT_SECT_NAME("Chip init data");

	ar5416_dump_eep_init(ini, sizeof(emp->init_data) / 2);
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
	if (eep_5416_get_rev(emp) >= AR5416_EEP_MINOR_VER_17) {
		printf("%-30s : %s\n", "Rx Gain Type",
		       pBase->rxGainType == 0 ? "23dB backoff" :
		       pBase->rxGainType == 1 ? "13dB backoff" :
		       pBase->rxGainType == 2 ? "original" :
		       "unknown");
	}
	if (eep_5416_get_rev(emp) >= AR5416_EEP_MINOR_VER_19) {
		printf("%-30s : %s\n", "Tx Gain Type",
		       pBase->txGainType == 0 ? "original" :
		       pBase->rxGainType == 1 ? "high power" :
		       "unknown");
	}
	if (eep_5416_get_rev(emp) >= AR5416_EEP_MINOR_VER_21) {
		printf("%-30s : %d\n", "Power table offset, dBm",
		       pBase->power_table_offset);
	}

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
#define PR_LINE(_token, _cb, ...)				\
	do {							\
		printf("%-33s :", _token);			\
		if (pBase->opCapFlags & AR5416_OPFLAGS_11G) {	\
			_cb(ar5416Eep->modalHeader2G, ## __VA_ARGS__);\
			printf("       %-20s", buf);		\
		}						\
		if (pBase->opCapFlags & AR5416_OPFLAGS_11A) {	\
			_cb(ar5416Eep->modalHeader5G, ## __VA_ARGS__);\
			printf("  %s", buf);			\
		}						\
		printf("\n");					\
	} while(0)
#define __HALFDB2DB(_val)	((_val) / (double)2.0)
#define __100NS2US(_val)	((_val) / (double)10.0)
#define __PR_FMT_CONV(_fmt, _val, _conv)			\
		snprintf(buf, sizeof(buf), _fmt, _conv(_val))
#define __PR_FMT(_fmt, _val)	__PR_FMT_CONV(_fmt, _val, )
#define __PR_FMT_PERCHAIN_CONV(_fmt, _val, _conv)		\
		snprintf(buf, sizeof(buf), _fmt " / " _fmt " / " _fmt,\
			 _conv(_val[0]), _conv(_val[1]), _conv(_val[2]))
#define __PR_FMT_PERCHAIN(_fmt, _val)				\
		__PR_FMT_PERCHAIN_CONV(_fmt, _val, )
#define _PR_CB_FMT_PERCHAIN(_hdr, _field, _fmt)			\
		__PR_FMT_PERCHAIN(_fmt, _hdr._field)
#define _PR_CB_DEC(_hdr, _field)				\
		__PR_FMT("%d", _hdr._field)
#define _PR_CB_DEC_PERCHAIN(_hdr, _field)			\
		__PR_FMT_PERCHAIN("%d", _hdr._field)
#define _PR_CB_HEX(_hdr, _field)				\
		__PR_FMT("0x%X", _hdr._field)
#define _PR_CB_PWR(_hdr, _field)				\
		__PR_FMT_CONV("%.1f", _hdr._field, __HALFDB2DB)
#define _PR_CB_PWR_PERCHAIN(_hdr, _field)			\
		__PR_FMT_PERCHAIN_CONV("%3.1f", _hdr._field, __HALFDB2DB)
#define _PR_CB_TIME(_hdr, _field)				\
		__PR_FMT_CONV("%.1f", _hdr._field, __100NS2US)
#define _PR_CB_FLAG(_hdr, _field, _false_str, _true_str)	\
		snprintf(buf, sizeof(buf), "%s",		\
			(_hdr._field) ? _true_str : _false_str)
#define _PR_CB_ANTCTRLCHAIN(_hdr, _field)			\
		do {						\
			uint16_t val = _hdr._field;		\
			snprintf(buf, sizeof(buf), "%x/%x/%x/%x/%x/%x",\
				 MS(val, AR5416_ANTCTRLCHAIN_IDLE),\
				 MS(val, AR5416_ANTCTRLCHAIN_TX),\
				 MS(val, AR5416_ANTCTRLCHAIN_RX_NOATT),\
				 MS(val, AR5416_ANTCTRLCHAIN_RX_ATT1),\
				 MS(val, AR5416_ANTCTRLCHAIN_RX_ATT12),\
				 MS(val, AR5416_ANTCTRLCHAIN_BT));\
		} while (0);
#define _PR_CB_ANTCTRLCMN(_hdr, _field)				\
		do {						\
			uint16_t val = _hdr._field;		\
			snprintf(buf, sizeof(buf), "%x / %x / %x / %x",\
				 MS(val, AR5416_ANTCTRLCMN_IDLE),\
				 MS(val, AR5416_ANTCTRLCMN_TX),	\
				 MS(val, AR5416_ANTCTRLCMN_RX),	\
				 MS(val, AR5416_ANTCTRLCMN_BT));\
		} while (0)
#define PR_DEC(_token, _field)					\
		PR_LINE(_token, _PR_CB_DEC, _field)
#define PR_DEC_PERCHAIN(_token, _field)				\
		PR_LINE(_token, _PR_CB_DEC_PERCHAIN, _field)
#define PR_HEX(_token, _field)					\
		PR_LINE(_token, _PR_CB_HEX, _field)
#define PR_PWR(_token, _field)					\
		PR_LINE(_token, _PR_CB_PWR, _field)
#define PR_PWR_PERCHAIN(_token, _field)				\
		PR_LINE(_token, _PR_CB_PWR_PERCHAIN, _field)
#define PR_TIME(_token, _field)					\
		PR_LINE(_token, _PR_CB_TIME, _field)
#define PR_FLAG(_token, _field, _false_str, _true_str)		\
		PR_LINE(_token, _PR_CB_FLAG, _field, _false_str, _true_str)
#define PR_FMT_PERCHAIN(_token, _field, _fmt)			\
		PR_LINE(_token, _PR_CB_FMT_PERCHAIN, _field, _fmt)

	struct eep_5416_priv *emp = aem->eepmap_priv;
	struct ar5416_eeprom *ar5416Eep = &emp->eep;
	struct ar5416_base_eep_hdr *pBase = &ar5416Eep->baseEepHeader;
	char buf[0x20];

	EEP_PRINT_SECT_NAME("EEPROM Modal Header");

	printf("%35s", "");
	if (pBase->opCapFlags & AR5416_OPFLAGS_11G)
		printf("       %-20s", "2G");
	if (pBase->opCapFlags & AR5416_OPFLAGS_11A)
		printf("  %s", "5G");
	printf("\n\n");

	PR_HEX("Ant Ctrl Chain 0", antCtrlChain[0]);
	PR_LINE("  Idle/Tx/Rx/RxAtt1/RxAtt1&2/BT", _PR_CB_ANTCTRLCHAIN,
		antCtrlChain[0]);
	PR_HEX("Ant Ctrl Chain 1", antCtrlChain[1]);
	PR_LINE("  Idle/Tx/Rx/RxAtt1/RxAtt1&2/BT", _PR_CB_ANTCTRLCHAIN,
		antCtrlChain[1]);
	PR_HEX("Ant Ctrl Chain 2", antCtrlChain[2]);
	PR_LINE("  Idle/Tx/Rx/RxAtt1/RxAtt1&2/BT", _PR_CB_ANTCTRLCHAIN,
		antCtrlChain[2]);
	PR_HEX("Antenna Ctrl Common", antCtrlCommon);
	PR_LINE("  Idle/Tx/Rx/BT", _PR_CB_ANTCTRLCMN, antCtrlCommon);
	PR_PWR_PERCHAIN("Antenna Gain (per-chain)", antennaGainCh);
	PR_DEC("Switch Settling", switchSettling);
	PR_FMT_PERCHAIN("TxRxAttenuation (per-chain), dB", txRxAttenCh, "%2u");
	PR_FMT_PERCHAIN("TxRxAtten margin (per-chain), dB", rxTxMarginCh, "%2u");
	if (eep_5416_get_rev(emp) >= AR5416_EEP_MINOR_VER_3) {
		PR_FMT_PERCHAIN("bswAtten (per-chain), dB", bswAtten,
				"%2u");
		PR_FMT_PERCHAIN("bswAtten margin (per-chain), dB", bswMargin,
				"%2u");
	}
	if (AR_SREV_9280_20_OR_LATER(aem)) {
		PR_FMT_PERCHAIN("xatten2Db (per-chain)", xatten2Db, "%2u");
		PR_FMT_PERCHAIN("xatten2Db margin (per-chain)", xatten2Margin,
				"%2u");
	}
	PR_PWR("ADC Desired Size, dBm", adcDesiredSize);
	PR_PWR("PGA Desired Size, dBm", pgaDesiredSize);
	PR_DEC_PERCHAIN("xLNA gain (per-chain)", xlnaGainCh);
	PR_DEC("Thresh62", thresh62);
	PR_DEC_PERCHAIN("NF Thresh (per-chain)", noiseFloorThreshCh);
	PR_HEX("xPD Gain Mask", xpdGain);
	PR_FLAG("PD type", xpd, "internal", "external");
	PR_DEC_PERCHAIN("IQ Cal I (per-chain)", iqCalICh);
	PR_DEC_PERCHAIN("IQ Cal Q (per-chain)", iqCalQCh);
	PR_DEC("Analog Output Bias(ob)", ob);
	PR_DEC("Analog Driver Bias(db)", db);
	PR_DEC("xPA bias level", xpaBiasLvl);
	PR_DEC("xPA bias level Freq 0", xpaBiasLvlFreq[0]);
	PR_DEC("xPA bias level Freq 1", xpaBiasLvlFreq[1]);
	PR_DEC("xPA bias level Freq 2", xpaBiasLvlFreq[2]);
	PR_HEX("xLNA control", lna_ctl);

	PR_PWR("PD gain Overlap, dB", pdGainOverlap);
	PR_PWR("Pwr decrease 2 chain", pwrDecreaseFor2Chain);
	PR_PWR("Pwr decrease 3 chain", pwrDecreaseFor3Chain);

	if (AR_SREV_9280_20_OR_LATER(aem)) {
		PR_DEC("ob_ch1", ob_ch1);
		PR_DEC("db_ch1", db_ch1);
	}

	if (eep_5416_get_rev(emp) >= AR5416_EEP_MINOR_VER_3)
		PR_DEC("HT40PowerIncForPDADC", ht40PowerIncForPdadc);

	PR_TIME("TX End to xLNA On, us", txEndToRxOn);
	PR_TIME("TX End to xPA Off, us", txEndToXpaOff);
	PR_TIME("TX Frame to xPA On, us", txFrameToXpaOn);
	if (eep_5416_get_rev(emp) >= AR5416_EEP_MINOR_VER_3) {
		PR_TIME("TX Frame to DataStart, us", txFrameToDataStart);
		PR_TIME("TX Frame to PA On, us", txFrameToPaOn);
	}

	printf("\n");

#undef PR_FMT_PERCHAIN
#undef PR_FLAG
#undef PR_TIME
#undef PR_PWR_PERCHAIN
#undef PR_PWR
#undef PR_HEX
#undef PR_DEC_PERCHAIN
#undef PR_DEC
#undef _PR_CB_ANTCTRLCMN
#undef _PR_CB_ANTCTRLCHAIN
#undef _PR_CB_FLAG
#undef _PR_CB_TIME
#undef _PR_CB_PWR_PERCHAIN
#undef _PR_CB_PWR
#undef _PR_CB_HEX
#undef _PR_CB_DEC_PERCHAIN
#undef _PR_CB_DEC
#undef _PR_CB_FMT_PERCHAIN
#undef __PR_FMT_PERCHAIN
#undef __PR_FMT_PERCHAIN_CONV
#undef __PR_FMT
#undef __PR_FMT_CONV
#undef __100NS2US
#undef __HALFDB2DB
#undef PR_LINE
}

static void
eep_5416_dump_closeloop_item(const struct ar5416_cal_data_per_freq *item,
			     int gainmask, int power_table_offset)
{
	const char * const gains[AR5416_NUM_PD_GAINS] = {"4", "2", "1", "0.5"};
	struct {
		uint8_t pwr;
		uint8_t vpd[AR5416_NUM_PD_GAINS];
	} merged[AR5416_PD_GAIN_ICEPTS * AR5416_NUM_PD_GAINS];
	/* Map of Mask Gain bit Index to Calibrated per-Gain icepts set Index */
	int mgi2cgi[AR5416_NUM_PD_GAINS];
	int cgii[AR5416_NUM_PD_GAINS];	/* Array of indexes for merge */
	int gain, ngains, pwr, npwr;	/* Indexes */
	uint8_t pwrmin;

	/**
	 * Index of bits in the gains mask is not the same as gain index in the
	 * calibration data. Calibration data are stored without gaps. And non
	 * available per-gain sets are skipped if gain is not enabled via the
	 * gains mask.  E.g. if the gains mask have a value of 0x06 then you
	 * should use sets #0 and #1 from the calibration data. Where set #0 is
	 * corespond to gains mask bit #1 and set #1 coresponds to gains mask
	 * bit #3.
	 *
	 * To simplify further code we build a map of gain indexes to
	 * calibration data sets indexes using the gains mask. Also count a
	 * number of gains mask bit that are set aka number of configured
	 * gains.
	 */
	ngains = 0;
	for (gain = 0; gain < AR5416_NUM_PD_GAINS; ++gain) {
		if (gainmask & (1 << gain)) {
			mgi2cgi[gain] = ngains;
			ngains++;
		} else {
			mgi2cgi[gain] = -1;
		}
	}

	/* Merge calibration per-gain power lists to filter duplicates */
	memset(merged, 0xff, sizeof(merged));
	memset(cgii, 0x00, sizeof(cgii));
	for (pwr = 0; pwr < ARRAY_SIZE(merged); ++pwr) {
		pwrmin = 0xff;
		/* Looking for unmerged yet power value */
		for (gain = 0; gain < ngains; ++gain) {
			if (cgii[gain] >= AR5416_PD_GAIN_ICEPTS)
				continue;
			if (item->pwrPdg[gain][cgii[gain]] < pwrmin)
				pwrmin = item->pwrPdg[gain][cgii[gain]];
		}
		if (pwrmin == 0xff)
			break;
		merged[pwr].pwr = pwrmin;
		/* Copy Vpd of all gains for this power */
		for (gain = 0; gain < ngains; ++gain) {
			if (cgii[gain] >= AR5416_PD_GAIN_ICEPTS ||
			    item->pwrPdg[gain][cgii[gain]] != pwrmin)
				continue;
			merged[pwr].vpd[gain] = item->vpdPdg[gain][cgii[gain]];
			cgii[gain]++;
		}
	}
	npwr = pwr;

	/* Print merged data */
	printf("      Tx Power, dBm:");
	for (pwr = 0; pwr < npwr; ++pwr)
		printf(" %5.2f", (double)merged[pwr].pwr / 4 +
				 power_table_offset);
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
			uint8_t vpd = merged[pwr].vpd[mgi2cgi[gain]];

			if (vpd == 0xff)
				printf("      ");
			else
				printf("   %3u", vpd);
		}
		printf("\n");
	}
}

static void eep_5416_dump_closeloop(const uint8_t *freqs, int maxfreq,
				    const struct ar5416_cal_data_per_freq *cal,
				    int is_2g, int chainmask, int gainmask,
				    int power_table_offset)
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

			eep_5416_dump_closeloop_item(item, gainmask,
						     power_table_offset);

			printf("\n");
		}
	}
}

static void eep_5416_dump_pd_cal(const uint8_t *freq, int maxfreq,
				 const void *caldata, int is_openloop,
				 int is_2g, int chainmask, int gainmask,
				 int power_table_offset)
{
	if (is_openloop) {
		printf("  Open-loop PD calibration dumping is not supported\n");
	} else {
		eep_5416_dump_closeloop(freq, maxfreq, caldata, is_2g,
					chainmask, gainmask,
					power_table_offset);
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
				     (eep->modalHeader ## __band).xpdGain, \
				     power_table_offset);\
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
	int power_table_offset;

	EEP_PRINT_SECT_NAME("EEPROM Power Info");

	if (eep_5416_get_rev(emp) >= AR5416_EEP_MINOR_VER_19 &&
	    eep->baseEepHeader.openLoopPwrCntl & 0x01)
		is_openloop = 1;

	power_table_offset = eep_5416_get_rev(emp) >= AR5416_EEP_MINOR_VER_21 ?
			     eep->baseEepHeader.power_table_offset :
			     AR5416_PWR_TABLE_OFFSET_DB;

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
#ifdef CONFIG_I_KNOW_WHAT_I_AM_DOING
	case EEP_ERASE_CTL:
		/* It is enough to erase the CTL index only */
		data_pos = AR5416_DATA_START_LOC + EEP_FIELD_OFFSET(ctlIndex);
		data_len = EEP_FIELD_SIZE(ctlIndex);

		/**
		 * The bad news is that the CTL index starts in the middle of
		 * a EEPROM word (its bytes offset is 0x0a81). Since the CTL
		 * index have an integer number of EEPROM words (0x18 bytes,
		 * 0x0c words) it ends also in a middle of the word (bytes
		 * offset 0x0a98). So in fact we should update +1 EEPROM words
		 * to cover the whole CTL index. Also we need a special
		 * treatment on first and last words erasing to avoid erasing
		 * data that share these EEPROM words with the CTL index.
		 */

		data_len += 1;		/* Extend updation range */

		/**
		 * On a Little-Endians machine this code is equal to:
		 *   memset((uint8_t *)buf + data_pos * 2 + 1, 0x00,
		 *          (data_len - 1) * 2)
		 *
		 * But the following code should give a better representation of
		 * the operation from the EEPROM point of view. Also this code
		 * will work on any machine (i.e. having any endians).
		 */
		addr = data_pos;
		buf[addr] &= 0x00ff;	/* Erase word MSB (i.e. first entry) */
		for (++addr; addr < (data_pos + data_len - 1); ++addr)
			buf[addr] = 0x0000;
		buf[addr] &= 0xff00;	/* Erase word LSB (i.e. last entry) */
		break;
#endif
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
	.load_eeprom  = eep_5416_load_eeprom,
	.check_eeprom = eep_5416_check,
	.dump = {
		[EEP_SECT_INIT] = eep_5416_dump_init_data,
		[EEP_SECT_BASE] = eep_5416_dump_base_header,
		[EEP_SECT_MODAL] = eep_5416_dump_modal_header,
		[EEP_SECT_POWER] = eep_5416_dump_power_info,
	},
	.update_eeprom = eep_5416_update_eeprom,
	.params_mask = BIT(EEP_UPDATE_MAC)
#ifdef CONFIG_I_KNOW_WHAT_I_AM_DOING
		| BIT(EEP_ERASE_CTL)
#endif
	,
};
