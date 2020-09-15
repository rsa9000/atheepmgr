/*
 * Copyright (c) 2020 Sergey Ryazanov <ryazanov.s.a@gmail.com>
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

#ifndef EEP_9880_H
#define EEP_9880_H

#define QCA9880_MAX_CHAINS			3

#define QCA9880_OPFLAGS_11A			0x01
#define QCA9880_OPFLAGS_11G			0x02
#define QCA9880_OPFLAGS_5G_HT40			0x04
#define QCA9880_OPFLAGS_2G_HT40			0x08
#define QCA9880_OPFLAGS_5G_HT20			0x10
#define QCA9880_OPFLAGS_2G_HT20			0x20

#define QCA9880_OPFLAGS2_5G_VHT20		0x01
#define QCA9880_OPFLAGS2_2G_VHT20		0x02
#define QCA9880_OPFLAGS2_5G_VHT40		0x04
#define QCA9880_OPFLAGS2_2G_VHT40		0x08
#define QCA9880_OPFLAGS2_5G_VHT80		0x10

#define QCA9880_FEATURE_TEMP_COMP		0x01
#define QCA9880_FEATURE_VOLT_COMP		0x02
#define QCA9880_FEATURE_DOUBLING		0x04
#define QCA9880_FEATURE_INT_REGULATOR		0x08
#define QCA9880_FEATURE_TUNING_CAPS		0x40

#define QCA9880_CUSTOMER_DATA_SIZE		20

#define QCA9880_NUM_2G_CAL_PIERS		3
#define QCA9880_NUM_5G_CAL_PIERS		8
#define QCA9880_NUM_CAL_GAINS			2

#define QCA9880_TGTPWR_CCK_2G_NUM_FREQS		2
#define QCA9880_TGTPWR_LEG_2G_NUM_FREQS		3
#define QCA9880_TGTPWR_VHT_2G_NUM_FREQS		3
#define QCA9880_TGTPWR_LEG_5G_NUM_FREQS		6
#define QCA9880_TGTPWR_VHT_5G_NUM_FREQS		6
#define QCA9880_TGTPWR_VHT_NUM_RATES		18
#define QCA9880_TGTPWR_VHT_2GVHT20_BWIDX	0
#define QCA9880_TGTPWR_VHT_2GVHT40_BWIDX	1
#define QCA9880_TGTPWR_VHT_5GVHT20_BWIDX	0
#define QCA9880_TGTPWR_VHT_5GVHT40_BWIDX	1
#define QCA9880_TGTPWR_VHT_5GVHT80_BWIDX	2
#define QCA9880_TGTPWR_VHT_2G_NUM_BWS		2	/* VHT: 20, 40 */
#define QCA9880_TGTPWR_VHT_5G_NUM_BWS		3	/* VHT: 20, 40, 80 */
#define QCA9880_TGTPWR_VHT_2G_EXT_DELTA_BITS		\
		(QCA9880_TGTPWR_VHT_2G_NUM_BWS *	\
		 QCA9880_TGTPWR_VHT_2G_NUM_FREQS *	\
		 QCA9880_TGTPWR_VHT_NUM_RATES)
#define QCA9880_TGTPWR_VHT_2G_EXT_DELTA_SIZE		\
		((QCA9880_TGTPWR_VHT_2G_EXT_DELTA_BITS + 7) / 8)
#define QCA9880_TGTPWR_VHT_5G_EXT_DELTA_BITS		\
		(QCA9880_TGTPWR_VHT_5G_NUM_BWS *	\
		 QCA9880_TGTPWR_VHT_5G_NUM_FREQS *	\
		 QCA9880_TGTPWR_VHT_NUM_RATES)
#define QCA9880_TGTPWR_VHT_5G_EXT_DELTA_SIZE		\
		((QCA9880_TGTPWR_VHT_5G_EXT_DELTA_BITS + 7) / 8)

#define QCA9880_NUM_2G_CTLS			18
#define QCA9880_NUM_2G_BAND_EDGES		4
#define QCA9880_NUM_5G_CTLS			18
#define QCA9880_NUM_5G_BAND_EDGES		8

#define QCA9880_NUM_2G_ALPHATHERM_CHANS		4
#define QCA9880_NUM_5G_ALPHATHERM_CHANS		8
#define QCA9880_NUM_ALPHATHERM_TEMPS		4

#define QCA9880_CONFIG_ENTRIES			24

/* Almost arbitrary value, bytes */
#define QCA9880_EEPROM_SIZE			0x1000

struct qca9880_eep_flags {
	uint8_t opFlags;
	uint8_t featureFlags;
	uint8_t miscFlags;
	uint8_t __unkn_03;
	uint32_t boardFlags;
	uint8_t __unkn_08[2];
	uint8_t opFlags2;
	uint8_t __unkn_0b[1];
} __attribute__ ((packed));

struct qca9880_base_eep_hdr {
	uint16_t length;
	uint16_t checksum;
	uint8_t eepromVersion;
	uint8_t templateVersion;
	uint8_t macAddr[6];
	uint16_t regDmn[2];
	struct qca9880_eep_flags opCapBrdFlags;
	uint16_t binBuildNumber;
	uint8_t txrxMask;
	uint8_t rfSilent;
	uint8_t wlanLedGpio;
	uint8_t spurBaseA;
	uint8_t spurBaseB;
	uint8_t spurRssiThresh;
	uint8_t spurRssiThreshCck;
	uint8_t spurMitFlag;
	uint8_t swreg;
	uint8_t txrxgain;
	int8_t pwrTableOffset;
	uint8_t param_for_tuning_caps;
	int8_t deltaCck20;
	int8_t delta4020;
	int8_t delta8020;
	uint8_t custData[QCA9880_CUSTOMER_DATA_SIZE];
	uint8_t param_for_tuning_caps1;
	uint8_t futureBase[66];
} __attribute__ ((packed));

struct qca9880_spur_chan {
	uint8_t spurChan;
	uint8_t spurA_PrimSecChoose;
	uint8_t spurB_PrimSecChoose;
} __attribute__ ((packed));

struct qca9880_modal_eep_hdr {
	int8_t voltSlope[QCA9880_MAX_CHAINS];
	struct qca9880_spur_chan spurChans[AR5416_EEPROM_MODAL_SPURS];
	uint8_t xpaBiasLvl;
	int8_t antennaGain;
	uint32_t antCtrlCommon;
	uint32_t antCtrlCommon2;
	uint16_t antCtrlChain[QCA9880_MAX_CHAINS];
	uint8_t rxFilterCap;
	uint8_t rxGainCap;
	uint8_t txrxgain;
	int8_t noiseFloorThresh;
	int8_t minCcaPwr[QCA9880_MAX_CHAINS];
	uint8_t futureModal[123];
} __attribute__ ((packed));

struct qca9880_modal_piers {
	uint8_t value2G;
	uint8_t value5GLow;
	uint8_t value5GMid;
	uint8_t value5GHigh;
} __attribute__ ((packed));

struct qca9880_base_extention {
	struct qca9880_modal_piers xatten1DB[QCA9880_MAX_CHAINS];
	struct qca9880_modal_piers xatten1Margin[QCA9880_MAX_CHAINS];
	struct qca9880_modal_piers reserved[QCA9880_MAX_CHAINS * 5];
} __attribute__ ((packed));

struct qca9880_therm_cal {
	uint16_t thermAdcScaledGain;
	int8_t thermAdcOffset;
	uint8_t rbias;
} __attribute__ ((packed));

struct qca9880_cal_data_per_chain {
	uint8_t txgainIdx[QCA9880_NUM_CAL_GAINS];
	uint16_t power[QCA9880_NUM_CAL_GAINS];
} __attribute__ ((packed));

struct qca9880_cal_data_per_freq_op_loop {
	struct qca9880_cal_data_per_chain calPerChain[QCA9880_MAX_CHAINS];
	int8_t dacGain[QCA9880_NUM_CAL_GAINS];
	uint8_t thermCalVal;
	uint8_t voltCalVal;
} __attribute__ ((packed));

struct qca9880_cal_tgt_pow_legacy {
	uint8_t tPow2x[4];
} __attribute__ ((packed));

struct qca9880_cal_tgt_pow_vht {
	uint8_t tPow2xBase[QCA9880_MAX_CHAINS];
	uint8_t tPow2xDelta[QCA9880_TGTPWR_VHT_NUM_RATES / 2];
} __attribute__ ((packed));

struct qca9880_eeprom {
	struct qca9880_base_eep_hdr baseEepHeader;
	struct qca9880_modal_eep_hdr modalHeader5G;
	struct qca9880_modal_eep_hdr modalHeader2G;
	struct qca9880_base_extention baseExt;
	struct qca9880_therm_cal thermCal;

	uint8_t calFreqPier2G[QCA9880_NUM_2G_CAL_PIERS];
	uint8_t __pad_0227[1];
	struct qca9880_cal_data_per_freq_op_loop
	  calPierData2G[QCA9880_NUM_2G_CAL_PIERS];
	uint8_t futureCalData2G[46];

	uint8_t extTPow2xDelta2G[QCA9880_TGTPWR_VHT_2G_EXT_DELTA_SIZE];
	uint8_t __pad_02a6[2];
	uint8_t targetFreqbin2GCck[QCA9880_TGTPWR_CCK_2G_NUM_FREQS];
	uint8_t targetFreqbin2GLeg[QCA9880_TGTPWR_LEG_2G_NUM_FREQS];
	uint8_t __pad_02ad[1];
	uint8_t targetFreqbin2GVHT20[QCA9880_TGTPWR_VHT_2G_NUM_FREQS];
	uint8_t __pad_02b1[1];
	uint8_t targetFreqbin2GVHT40[QCA9880_TGTPWR_VHT_2G_NUM_FREQS];
	uint8_t __pad_02b5[1];
	struct qca9880_cal_tgt_pow_legacy
	  targetPower2GCck[QCA9880_TGTPWR_CCK_2G_NUM_FREQS];
	struct qca9880_cal_tgt_pow_legacy
	  targetPower2GLeg[QCA9880_TGTPWR_LEG_2G_NUM_FREQS];
	struct qca9880_cal_tgt_pow_vht
	  targetPower2GVHT20[QCA9880_TGTPWR_VHT_2G_NUM_FREQS];
	struct qca9880_cal_tgt_pow_vht
	  targetPower2GVHT40[QCA9880_TGTPWR_VHT_2G_NUM_FREQS];

	uint8_t ctlIndex2G[QCA9880_NUM_2G_CTLS];
	uint8_t __pad_0324[2];
	uint8_t ctlFreqBin2G[QCA9880_NUM_2G_CTLS][QCA9880_NUM_2G_BAND_EDGES];
	uint8_t ctlData2G[QCA9880_NUM_2G_CTLS][QCA9880_NUM_2G_BAND_EDGES];
	uint8_t futureCtl2G[40];

	uint8_t alphaThermTbl2G[QCA9880_MAX_CHAINS][QCA9880_NUM_2G_ALPHATHERM_CHANS][QCA9880_NUM_ALPHATHERM_TEMPS];
	uint8_t __pad_040e[2];

	uint8_t calFreqPier5G[QCA9880_NUM_5G_CAL_PIERS];
	struct qca9880_cal_data_per_freq_op_loop
	  calPierData5G[QCA9880_NUM_5G_CAL_PIERS];
	uint8_t futureCalData5G[20];

	uint8_t extTPow2xDelta5G[QCA9880_TGTPWR_VHT_5G_EXT_DELTA_SIZE];
	uint8_t __pad_0505[3];
	uint8_t targetFreqbin5GLeg[QCA9880_TGTPWR_LEG_5G_NUM_FREQS];
	uint8_t targetFreqbin5GVHT20[QCA9880_TGTPWR_VHT_5G_NUM_FREQS];
	uint8_t targetFreqbin5GVHT40[QCA9880_TGTPWR_VHT_5G_NUM_FREQS];
	uint8_t targetFreqbin5GVHT80[QCA9880_TGTPWR_VHT_5G_NUM_FREQS];
	struct qca9880_cal_tgt_pow_legacy
	  targetPower5GLeg[QCA9880_TGTPWR_LEG_5G_NUM_FREQS];
	struct qca9880_cal_tgt_pow_vht
	  targetPower5GVHT20[QCA9880_TGTPWR_VHT_5G_NUM_FREQS];
	struct qca9880_cal_tgt_pow_vht
	  targetPower5GVHT40[QCA9880_TGTPWR_VHT_5G_NUM_FREQS];
	struct qca9880_cal_tgt_pow_vht
	  targetPower5GVHT80[QCA9880_TGTPWR_VHT_5G_NUM_FREQS];

	uint8_t ctlIndex5G[QCA9880_NUM_5G_CTLS];
	uint8_t __pad_0622[2];
	uint8_t ctlFreqBin5G[QCA9880_NUM_5G_CTLS][QCA9880_NUM_5G_BAND_EDGES];
	uint8_t ctlData5G[QCA9880_NUM_5G_CTLS][QCA9880_NUM_5G_BAND_EDGES];
	uint8_t futureCtl5G[64];

	uint8_t alphaThermTbl5G[QCA9880_MAX_CHAINS][QCA9880_NUM_5G_ALPHATHERM_CHANS][QCA9880_NUM_ALPHATHERM_TEMPS];

	uint32_t configAddr[QCA9880_CONFIG_ENTRIES];
} __attribute__ ((packed));

#endif
