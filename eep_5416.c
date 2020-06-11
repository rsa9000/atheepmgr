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
	if (eep_5416_get_rev(emp) >= AR5416_EEP_MINOR_VER_17) {
		printf("%-30s : %s\n", "Rx Gain Type",
		       pBase->rxGainType == 0 ? "23dB backoff" :
		       pBase->rxGainType == 1 ? "13dB backoff" :
		       pBase->rxGainType == 2 ? "original" :
		       "unknown");
	}
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

	PR_HEX("Ant Chain 0", antCtrlChain[0]);
	PR_LINE("  Idle/Tx/Rx/RxAtt1/RxAtt1&2/BT", _PR_CB_ANTCTRLCHAIN,
		antCtrlChain[0]);
	PR_HEX("Ant Chain 1", antCtrlChain[1]);
	PR_LINE("  Idle/Tx/Rx/RxAtt1/RxAtt1&2/BT", _PR_CB_ANTCTRLCHAIN,
		antCtrlChain[1]);
	PR_HEX("Ant Chain 2", antCtrlChain[2]);
	PR_LINE("  Idle/Tx/Rx/RxAtt1/RxAtt1&2/BT", _PR_CB_ANTCTRLCHAIN,
		antCtrlChain[2]);
	PR_HEX("Antenna Common", antCtrlCommon);
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
	PR_DEC("TX end to xlna on", txEndToRxOn);
	PR_DEC_PERCHAIN("xLNA gain (per-chain)", xlnaGainCh);
	PR_DEC("TX end to xpa off", txEndToXpaOff);
	PR_DEC("TX frame to xpa on", txFrameToXpaOn);
	PR_DEC("THRESH62", thresh62);
	PR_DEC_PERCHAIN("NF Thresh (per-chain)", noiseFloorThreshCh);
	PR_HEX("Xpd Gain Mask", xpdGain);
	PR_DEC("Xpd", xpd);
	PR_DEC_PERCHAIN("IQ Cal I (per-chain)", iqCalICh);
	PR_DEC_PERCHAIN("IQ Cal Q (per-chain)", iqCalQCh);
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
		PR_DEC("ob_ch1", ob_ch1);
		PR_DEC("db_ch1", db_ch1);
	}

	if (eep_5416_get_rev(emp) >= AR5416_EEP_MINOR_VER_3) {
		PR_DEC("txFrameToDataStart", txFrameToDataStart);
		PR_DEC("txFrameToPaOn", txFrameToPaOn);
		PR_DEC("HT40PowerIncForPDADC", ht40PowerIncForPdadc);
	}

	printf("\n");

#undef PR_FMT_PERCHAIN
#undef PR_PWR_PERCHAIN
#undef PR_PWR
#undef PR_HEX
#undef PR_DEC_PERCHAIN
#undef PR_DEC
#undef _PR_CB_ANTCTRLCMN
#undef _PR_CB_ANTCTRLCHAIN
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
#undef __HALFDB2DB
#undef PR_LINE
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
