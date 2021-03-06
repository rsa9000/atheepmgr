/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * Copyright (c) 2018-2020 Sergey Ryazanov <ryazanov.s.a@gmail.com>
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

#ifndef EEP_9300_TEMPLATES_H
#define EEP_9300_TEMPLATES_H

/**
 * gcc does not like to initialize from structure field, so define template
 * versions independently and initialize both template and index with enum
 * values. Use lower case to facilitate index filling with macro.
 */
enum ar9300_template_versions {
	ar9300_tpl_ver_default = 2,
	ar9300_tpl_ver_h112 = 3,
	ar9300_tpl_ver_h116 = 4,
	ar9300_tpl_ver_x112 = 5,
	ar9300_tpl_ver_x113 = 6,
};

static const struct ar9300_eeprom ar9300_default = {
	.eepromVersion = 2,
	.templateVersion = ar9300_tpl_ver_default,
	.macAddr = {0, 2, 3, 4, 5, 6},
	.custData = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		     0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	.baseEepHeader = {
		.regDmn = { LE16CONST(0x0000), LE16CONST(0x001f) },
		.txrxMask =  0x77, /* 4 bits tx and 4 bits rx */
		.opCapFlags = {
			.opFlags = AR5416_OPFLAGS_11G | AR5416_OPFLAGS_11A,
			.eepMisc = 0,
		},
		.rfSilent = 0,
		.blueToothOptions = 0,
		.deviceCap = 0,
		.deviceType = 5, /* takes lower byte in eeprom location */
		.pwrTableOffset = AR9300_PWR_TABLE_OFFSET,
		.params_for_tuning_caps = {0, 0},
		.featureEnable = 0x0c,
		/*
		 * bit0 - enable tx temp comp - disabled
		 * bit1 - enable tx volt comp - disabled
		 * bit2 - enable fastClock - enabled
		 * bit3 - enable doubling - enabled
		 * bit4 - enable internal regulator - disabled
		 * bit5 - enable pa predistortion - disabled
		 */
		.miscConfiguration = 0, /* bit0 - turn down drivestrength */
		.eepromWriteEnableGpio = 3,
		.wlanDisableGpio = 0,
		.wlanLedGpio = 8,
		.rxBandSelectGpio = 0xff,
		.txrxgain = 0,
		.swreg = 0,
	},
	.modalHeader2G = {
		/* ar9300_modal_eep_header  2g */
		.antCtrlCommon = LE32CONST(0x00000110),
		.antCtrlCommon2 = LE32CONST(0x00022222),
		.antCtrlChain = {
			LE16CONST(0x0150), LE16CONST(0x0150), LE16CONST(0x0150)
		},
		/*
		 * xatten1DB[AR9300_MAX_CHAINS];  3 xatten1_db
		 * for ar9280 (0xa20c/b20c 5:0)
		 */
		.xatten1DB = {0, 0, 0},

		/*
		 * xatten1Margin[AR9300_MAX_CHAINS]; 3 xatten1_margin
		 * for ar9280 (0xa20c/b20c 16:12
		 */
		.xatten1Margin = {0, 0, 0},
		.tempSlope = 36,
		.voltSlope = 0,

		/*
		 * spurChans[OSPREY_EEPROM_MODAL_SPURS]; spur
		 * channels in usual fbin coding format
		 */
		.spurChans = {0, 0, 0, 0, 0},

		/*
		 * noiseFloorThreshCh[AR9300_MAX_CHAINS]; 3 Check
		 * if the register is per chain
		 */
		.noiseFloorThreshCh = {-1, 0, 0},
		.reserved = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.quick_drop = 0,
		.xpaBiasLvl = 0,
		.txFrameToDataStart = 0x0e,
		.txFrameToPaOn = 0x0e,
		.txClip = 3, /* 4 bits tx_clip, 4 bits dac_scale_cck */
		.antennaGain = 0,
		.switchSettling = 0x2c,
		.adcDesiredSize = -30,
		.txEndToXpaOff = 0,
		.txEndToRxOn = 0x2,
		.txFrameToXpaOn = 0xe,
		.thresh62 = 28,
		.xlna_bias_strength = 0,
		.futureModal = {
			0, 0, 0, 0, 0, 0, 0,
		},
	},
	.base_ext1 = {
		.ant_div_control = 0,
		.future = {0, 0, 0},
		.tempslopextension = {0, 0, 0, 0, 0, 0, 0, 0}
	},
	.calFreqPier2G = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2472, 1),
	},
	/* ar9300_cal_data_per_freq_op_loop 2g */
	.calPierData2G = {
		{ {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0} },
		{ {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0} },
		{ {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0} },
	},
	.calTarget_freqbin_Cck = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2484, 1),
	},
	.calTarget_freqbin_2G = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2472, 1)
	},
	.calTarget_freqbin_2GHT20 = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2472, 1)
	},
	.calTarget_freqbin_2GHT40 = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2472, 1)
	},
	.calTargetPowerCck = {
		/* 1L-5L,5S,11L,11S */
		{ { PWR2X(18), PWR2X(18), PWR2X(18), PWR2X(18) } },
		{ { PWR2X(18), PWR2X(18), PWR2X(18), PWR2X(18) } }
	},
	.calTargetPower2G = {
		/* 6-24,36,48,54 */
		{ { PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(12) } },
		{ { PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(12) } },
		{ { PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(12) } }
	},
	.calTargetPower2GHT20 = {
		{{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(10),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(10),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(10)
		}}, {{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(10),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(10),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(10)
		}}, {{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(10),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(10),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(10)
		}}
	},
	.calTargetPower2GHT40 = {
		{{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(10),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(10),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(10)
		}}, {{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(10),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(10),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(10)
		}}, {{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(10),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(10),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(10)
		}}
	},
	.ctlIndex_2G =  {
		0x11, 0x12, 0x15, 0x17, 0x41, 0x42,
		0x45, 0x47, 0x31, 0x32, 0x35, 0x37,
	},
	.ctl_freqbin_2G = {
		{
			FREQ2FBIN(2412, 1),
			FREQ2FBIN(2417, 1),
			FREQ2FBIN(2457, 1),
			FREQ2FBIN(2462, 1)
		},
		{
			FREQ2FBIN(2412, 1),
			FREQ2FBIN(2417, 1),
			FREQ2FBIN(2462, 1),
			0xFF,
		},

		{
			FREQ2FBIN(2412, 1),
			FREQ2FBIN(2417, 1),
			FREQ2FBIN(2462, 1),
			0xFF,
		},
		{
			FREQ2FBIN(2422, 1),
			FREQ2FBIN(2427, 1),
			FREQ2FBIN(2447, 1),
			FREQ2FBIN(2452, 1)
		},

		{
			/* Data[4].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[4].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			/* Data[4].ctlEdges[2].bChannel */ FREQ2FBIN(2472, 1),
			/* Data[4].ctlEdges[3].bChannel */ FREQ2FBIN(2484, 1),
		},

		{
			/* Data[5].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[5].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			/* Data[5].ctlEdges[2].bChannel */ FREQ2FBIN(2472, 1),
			0,
		},

		{
			/* Data[6].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[6].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			FREQ2FBIN(2472, 1),
			0,
		},

		{
			/* Data[7].ctlEdges[0].bChannel */ FREQ2FBIN(2422, 1),
			/* Data[7].ctlEdges[1].bChannel */ FREQ2FBIN(2427, 1),
			/* Data[7].ctlEdges[2].bChannel */ FREQ2FBIN(2447, 1),
			/* Data[7].ctlEdges[3].bChannel */ FREQ2FBIN(2462, 1),
		},

		{
			/* Data[8].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[8].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			/* Data[8].ctlEdges[2].bChannel */ FREQ2FBIN(2472, 1),
		},

		{
			/* Data[9].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[9].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			/* Data[9].ctlEdges[2].bChannel */ FREQ2FBIN(2472, 1),
			0
		},

		{
			/* Data[10].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[10].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			/* Data[10].ctlEdges[2].bChannel */ FREQ2FBIN(2472, 1),
			0
		},

		{
			/* Data[11].ctlEdges[0].bChannel */ FREQ2FBIN(2422, 1),
			/* Data[11].ctlEdges[1].bChannel */ FREQ2FBIN(2427, 1),
			/* Data[11].ctlEdges[2].bChannel */ FREQ2FBIN(2447, 1),
			/* Data[11].ctlEdges[3].bChannel */ FREQ2FBIN(2462, 1),
		}
	},
	.ctlPowerData_2G = {
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 1) } },

		{ { CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },

		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },

		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1) } }
	},
	.modalHeader5G = {
		.antCtrlCommon = LE32CONST(0x00000110),
		.antCtrlCommon2 = LE32CONST(0x00022222),
		/* xatten1DB 3 xatten1_db for AR9280 (0xa20c/b20c 5:0) */
		.xatten1DB = {0, 0, 0},

		/*
		 * xatten1Margin[AR9300_MAX_CHAINS]; 3 xatten1_margin
		 * for merlin (0xa20c/b20c 16:12
		 */
		.xatten1Margin = {0, 0, 0},
		.tempSlope = 68,
		.voltSlope = 0,
		/* spurChans spur channels in usual fbin coding format */
		.spurChans = {0, 0, 0, 0, 0},
		/* noiseFloorThreshCh Check if the register is per chain */
		.noiseFloorThreshCh = {-1, 0, 0},
		.reserved = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.quick_drop = 0,
		.xpaBiasLvl = 0,
		.txFrameToDataStart = 0x0e,
		.txFrameToPaOn = 0x0e,
		.txClip = 3, /* 4 bits tx_clip, 4 bits dac_scale_cck */
		.antennaGain = 0,
		.switchSettling = 0x2d,
		.adcDesiredSize = -30,
		.txEndToXpaOff = 0,
		.txEndToRxOn = 0x2,
		.txFrameToXpaOn = 0xe,
		.thresh62 = 28,
		.xlna_bias_strength = 0,
		.futureModal = {
			0, 0, 0, 0, 0, 0, 0,
		},
	},
	.base_ext2 = {
		.tempSlopeLow = 0,
		.tempSlopeHigh = 0,
		.xatten1DBLow = {0, 0, 0},
		.xatten1MarginLow = {0, 0, 0},
		.xatten1DBHigh = {0, 0, 0},
		.xatten1MarginHigh = {0, 0, 0}
	},
	.calFreqPier5G = {
		FREQ2FBIN(5180, 0),
		FREQ2FBIN(5220, 0),
		FREQ2FBIN(5320, 0),
		FREQ2FBIN(5400, 0),
		FREQ2FBIN(5500, 0),
		FREQ2FBIN(5600, 0),
		FREQ2FBIN(5725, 0),
		FREQ2FBIN(5825, 0)
	},
	.calPierData5G = {
			{
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
			},
			{
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
			},
			{
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
			},

	},
	.calTarget_freqbin_5G = {
		FREQ2FBIN(5180, 0),
		FREQ2FBIN(5220, 0),
		FREQ2FBIN(5320, 0),
		FREQ2FBIN(5400, 0),
		FREQ2FBIN(5500, 0),
		FREQ2FBIN(5600, 0),
		FREQ2FBIN(5725, 0),
		FREQ2FBIN(5825, 0)
	},
	.calTarget_freqbin_5GHT20 = {
		FREQ2FBIN(5180, 0),
		FREQ2FBIN(5240, 0),
		FREQ2FBIN(5320, 0),
		FREQ2FBIN(5500, 0),
		FREQ2FBIN(5700, 0),
		FREQ2FBIN(5745, 0),
		FREQ2FBIN(5725, 0),
		FREQ2FBIN(5825, 0)
	},
	.calTarget_freqbin_5GHT40 = {
		FREQ2FBIN(5180, 0),
		FREQ2FBIN(5240, 0),
		FREQ2FBIN(5320, 0),
		FREQ2FBIN(5500, 0),
		FREQ2FBIN(5700, 0),
		FREQ2FBIN(5745, 0),
		FREQ2FBIN(5725, 0),
		FREQ2FBIN(5825, 0)
	},
	.calTargetPower5G = {
		/* 6-24,36,48,54 */
		{ {PWR2X(10), PWR2X(10), PWR2X(10), PWR2X(5)} },
		{ {PWR2X(10), PWR2X(10), PWR2X(10), PWR2X(5)} },
		{ {PWR2X(10), PWR2X(10), PWR2X(10), PWR2X(5)} },
		{ {PWR2X(10), PWR2X(10), PWR2X(10), PWR2X(5)} },
		{ {PWR2X(10), PWR2X(10), PWR2X(10), PWR2X(5)} },
		{ {PWR2X(10), PWR2X(10), PWR2X(10), PWR2X(5)} },
		{ {PWR2X(10), PWR2X(10), PWR2X(10), PWR2X(5)} },
		{ {PWR2X(10), PWR2X(10), PWR2X(10), PWR2X(5)} }
	},
	.calTargetPower5GHT20 = {
		/*
		 * 0_8_16, 1-3_9-11_17-19,
		 * 4,5,6,7,
		 * 12,13,14,15,
		 * 20,21,22,23
		 */
		{{
			PWR2X(10), PWR2X(10),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0)
		}}, {{
			PWR2X(10), PWR2X(10),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0)
		}}, {{
			PWR2X(10), PWR2X(10),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0)
		}}, {{
			PWR2X(10), PWR2X(10),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0)
		}}, {{
			PWR2X(10), PWR2X(10),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0)
		}}, {{
			PWR2X(10), PWR2X(10),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0)
		}}, {{
			PWR2X(10), PWR2X(10),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0)
		}}, {{
			PWR2X(10), PWR2X(10),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0)
		}}
	},
	.calTargetPower5GHT40 =  {
		/*
		 * 0_8_16, 1-3_9-11_17-19,
		 * 4,5,6,7,
		 * 12,13,14,15,
		 * 20,21,22,23
		 */
		{{
			PWR2X(10), PWR2X(10),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0)
		}}, {{
			PWR2X(10), PWR2X(10),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0)
		}}, {{
			PWR2X(10), PWR2X(10),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0)
		}}, {{
			PWR2X(10), PWR2X(10),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0)
		}}, {{
			PWR2X(10), PWR2X(10),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0)
		}}, {{
			PWR2X(10), PWR2X(10),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0)
		}}, {{
			PWR2X(10), PWR2X(10),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0)
		}}, {{
			PWR2X(10), PWR2X(10),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0),
			PWR2X(5), PWR2X(5), PWR2X(0), PWR2X(0)
		}}
	},
	.ctlIndex_5G =  {
		0x10, 0x16, 0x18, 0x40, 0x46,
		0x48, 0x30, 0x36, 0x38
	},
	.ctl_freqbin_5G =  {
		{
			/* Data[0].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[0].ctlEdges[1].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[0].ctlEdges[2].bChannel */ FREQ2FBIN(5280, 0),
			/* Data[0].ctlEdges[3].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[0].ctlEdges[4].bChannel */ FREQ2FBIN(5600, 0),
			/* Data[0].ctlEdges[5].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[0].ctlEdges[6].bChannel */ FREQ2FBIN(5745, 0),
			/* Data[0].ctlEdges[7].bChannel */ FREQ2FBIN(5825, 0)
		},
		{
			/* Data[1].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[1].ctlEdges[1].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[1].ctlEdges[2].bChannel */ FREQ2FBIN(5280, 0),
			/* Data[1].ctlEdges[3].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[1].ctlEdges[4].bChannel */ FREQ2FBIN(5520, 0),
			/* Data[1].ctlEdges[5].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[1].ctlEdges[6].bChannel */ FREQ2FBIN(5745, 0),
			/* Data[1].ctlEdges[7].bChannel */ FREQ2FBIN(5825, 0)
		},

		{
			/* Data[2].ctlEdges[0].bChannel */ FREQ2FBIN(5190, 0),
			/* Data[2].ctlEdges[1].bChannel */ FREQ2FBIN(5230, 0),
			/* Data[2].ctlEdges[2].bChannel */ FREQ2FBIN(5270, 0),
			/* Data[2].ctlEdges[3].bChannel */ FREQ2FBIN(5310, 0),
			/* Data[2].ctlEdges[4].bChannel */ FREQ2FBIN(5510, 0),
			/* Data[2].ctlEdges[5].bChannel */ FREQ2FBIN(5550, 0),
			/* Data[2].ctlEdges[6].bChannel */ FREQ2FBIN(5670, 0),
			/* Data[2].ctlEdges[7].bChannel */ FREQ2FBIN(5755, 0)
		},

		{
			/* Data[3].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[3].ctlEdges[1].bChannel */ FREQ2FBIN(5200, 0),
			/* Data[3].ctlEdges[2].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[3].ctlEdges[3].bChannel */ FREQ2FBIN(5320, 0),
			/* Data[3].ctlEdges[4].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[3].ctlEdges[5].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[3].ctlEdges[6].bChannel */ 0xFF,
			/* Data[3].ctlEdges[7].bChannel */ 0xFF,
		},

		{
			/* Data[4].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[4].ctlEdges[1].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[4].ctlEdges[2].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[4].ctlEdges[3].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[4].ctlEdges[4].bChannel */ 0xFF,
			/* Data[4].ctlEdges[5].bChannel */ 0xFF,
			/* Data[4].ctlEdges[6].bChannel */ 0xFF,
			/* Data[4].ctlEdges[7].bChannel */ 0xFF,
		},

		{
			/* Data[5].ctlEdges[0].bChannel */ FREQ2FBIN(5190, 0),
			/* Data[5].ctlEdges[1].bChannel */ FREQ2FBIN(5270, 0),
			/* Data[5].ctlEdges[2].bChannel */ FREQ2FBIN(5310, 0),
			/* Data[5].ctlEdges[3].bChannel */ FREQ2FBIN(5510, 0),
			/* Data[5].ctlEdges[4].bChannel */ FREQ2FBIN(5590, 0),
			/* Data[5].ctlEdges[5].bChannel */ FREQ2FBIN(5670, 0),
			/* Data[5].ctlEdges[6].bChannel */ 0xFF,
			/* Data[5].ctlEdges[7].bChannel */ 0xFF
		},

		{
			/* Data[6].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[6].ctlEdges[1].bChannel */ FREQ2FBIN(5200, 0),
			/* Data[6].ctlEdges[2].bChannel */ FREQ2FBIN(5220, 0),
			/* Data[6].ctlEdges[3].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[6].ctlEdges[4].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[6].ctlEdges[5].bChannel */ FREQ2FBIN(5600, 0),
			/* Data[6].ctlEdges[6].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[6].ctlEdges[7].bChannel */ FREQ2FBIN(5745, 0)
		},

		{
			/* Data[7].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[7].ctlEdges[1].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[7].ctlEdges[2].bChannel */ FREQ2FBIN(5320, 0),
			/* Data[7].ctlEdges[3].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[7].ctlEdges[4].bChannel */ FREQ2FBIN(5560, 0),
			/* Data[7].ctlEdges[5].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[7].ctlEdges[6].bChannel */ FREQ2FBIN(5745, 0),
			/* Data[7].ctlEdges[7].bChannel */ FREQ2FBIN(5825, 0)
		},

		{
			/* Data[8].ctlEdges[0].bChannel */ FREQ2FBIN(5190, 0),
			/* Data[8].ctlEdges[1].bChannel */ FREQ2FBIN(5230, 0),
			/* Data[8].ctlEdges[2].bChannel */ FREQ2FBIN(5270, 0),
			/* Data[8].ctlEdges[3].bChannel */ FREQ2FBIN(5510, 0),
			/* Data[8].ctlEdges[4].bChannel */ FREQ2FBIN(5550, 0),
			/* Data[8].ctlEdges[5].bChannel */ FREQ2FBIN(5670, 0),
			/* Data[8].ctlEdges[6].bChannel */ FREQ2FBIN(5755, 0),
			/* Data[8].ctlEdges[7].bChannel */ FREQ2FBIN(5795, 0)
		}
	},
	.ctlPowerData_5G = {
		{{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1)
		}}, {{
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0),
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0),
			CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1)
		}}
	}
};

static const struct ar9300_eeprom ar9300_x113 = {
	.eepromVersion = 2,
	.templateVersion = ar9300_tpl_ver_x113,
	.macAddr = {0x00, 0x03, 0x7f, 0x0, 0x0, 0x0},
	.custData = {"x113-023-f0000"},
	.baseEepHeader = {
		.regDmn = { LE16CONST(0x0000), LE16CONST(0x001f) },
		.txrxMask =  0x77, /* 4 bits tx and 4 bits rx */
		.opCapFlags = {
			.opFlags = AR5416_OPFLAGS_11A,
			.eepMisc = 0,
		},
		.rfSilent = 0,
		.blueToothOptions = 0,
		.deviceCap = 0,
		.deviceType = 5, /* takes lower byte in eeprom location */
		.pwrTableOffset = AR9300_PWR_TABLE_OFFSET,
		.params_for_tuning_caps = {0, 0},
		.featureEnable = 0x0d,
		/*
		 * bit0 - enable tx temp comp - disabled
		 * bit1 - enable tx volt comp - disabled
		 * bit2 - enable fastClock - enabled
		 * bit3 - enable doubling - enabled
		 * bit4 - enable internal regulator - disabled
		 * bit5 - enable pa predistortion - disabled
		 */
		.miscConfiguration = 0, /* bit0 - turn down drivestrength */
		.eepromWriteEnableGpio = 6,
		.wlanDisableGpio = 0,
		.wlanLedGpio = 8,
		.rxBandSelectGpio = 0xff,
		.txrxgain = 0x21,
		.swreg = 0,
	},
	.modalHeader2G = {
	/* ar9300_modal_eep_header  2g */
		.antCtrlCommon = LE32CONST(0x00000110),
		.antCtrlCommon2 = LE32CONST(0x00044444),
		.antCtrlChain = {
			LE16CONST(0x0150), LE16CONST(0x0150), LE16CONST(0x0150)
		},
		/*
		 * xatten1DB[AR9300_MAX_CHAINS];  3 xatten1_db
		 * for ar9280 (0xa20c/b20c 5:0)
		 */
		.xatten1DB = {0, 0, 0},

		/*
		 * xatten1Margin[AR9300_MAX_CHAINS]; 3 xatten1_margin
		 * for ar9280 (0xa20c/b20c 16:12
		 */
		.xatten1Margin = {0, 0, 0},
		.tempSlope = 25,
		.voltSlope = 0,

		/*
		 * spurChans[OSPREY_EEPROM_MODAL_SPURS]; spur
		 * channels in usual fbin coding format
		 */
		.spurChans = {FREQ2FBIN(2464, 1), 0, 0, 0, 0},

		/*
		 * noiseFloorThreshCh[AR9300_MAX_CHAINS]; 3 Check
		 * if the register is per chain
		 */
		.noiseFloorThreshCh = {-1, 0, 0},
		.reserved = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.quick_drop = 0,
		.xpaBiasLvl = 0,
		.txFrameToDataStart = 0x0e,
		.txFrameToPaOn = 0x0e,
		.txClip = 3, /* 4 bits tx_clip, 4 bits dac_scale_cck */
		.antennaGain = 0,
		.switchSettling = 0x2c,
		.adcDesiredSize = -30,
		.txEndToXpaOff = 0,
		.txEndToRxOn = 0x2,
		.txFrameToXpaOn = 0xe,
		.thresh62 = 28,
		.xlna_bias_strength = 0,
		.futureModal = {
			0, 0, 0, 0, 0, 0, 0,
		},
	},
	.base_ext1 = {
		.ant_div_control = 0,
		.future = {0, 0, 0},
		.tempslopextension = {0, 0, 0, 0, 0, 0, 0, 0}
	},
	.calFreqPier2G = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2472, 1),
	},
	/* ar9300_cal_data_per_freq_op_loop 2g */
	.calPierData2G = {
		{ {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0} },
		{ {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0} },
		{ {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0} },
	},
	.calTarget_freqbin_Cck = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2472, 1),
	},
	.calTarget_freqbin_2G = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2472, 1)
	},
	.calTarget_freqbin_2GHT20 = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2472, 1)
	},
	.calTarget_freqbin_2GHT40 = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2472, 1)
	},
	.calTargetPowerCck = {
		/* 1L-5L,5S,11L,11S */
		{ { PWR2X(17), PWR2X(17), PWR2X(17), PWR2X(17) } },
		{ { PWR2X(17), PWR2X(17), PWR2X(17), PWR2X(17) } }
	},
	.calTargetPower2G = {
		/* 6-24,36,48,54 */
		{ { PWR2X(17), PWR2X(17), PWR2X(16), PWR2X(16) } },
		{ { PWR2X(17), PWR2X(17), PWR2X(16), PWR2X(16) } },
		{ { PWR2X(17), PWR2X(17), PWR2X(16), PWR2X(16) } }
	},
	.calTargetPower2GHT20 = {
		{{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(16), PWR2X(16), PWR2X(14),
			PWR2X(16), PWR2X(16), PWR2X(15), PWR2X(14),
			PWR2X(0), PWR2X(0), PWR2X(0), PWR2X(0)
		}}, {{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(16), PWR2X(16), PWR2X(14),
			PWR2X(16), PWR2X(16), PWR2X(15), PWR2X(14),
			PWR2X(0), PWR2X(0), PWR2X(0), PWR2X(0)
		}}, {{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(16), PWR2X(16), PWR2X(14),
			PWR2X(16), PWR2X(16), PWR2X(15), PWR2X(14),
			PWR2X(0), PWR2X(0), PWR2X(0), PWR2X(0)
		}}
	},
	.calTargetPower2GHT40 = {
		{{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(15), PWR2X(15), PWR2X(14),
			PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(13),
			PWR2X(0), PWR2X(0), PWR2X(0), PWR2X(0)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(15), PWR2X(15), PWR2X(14),
			PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(13),
			PWR2X(0), PWR2X(0), PWR2X(0), PWR2X(0)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(15), PWR2X(15), PWR2X(14),
			PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(13),
			PWR2X(0), PWR2X(0), PWR2X(0), PWR2X(0)
		}}
	},
	.ctlIndex_2G =  {
		0x11, 0x12, 0x15, 0x17, 0x41, 0x42,
		0x45, 0x47, 0x31, 0x32, 0x35, 0x37,
	},
	.ctl_freqbin_2G = {
		{
			FREQ2FBIN(2412, 1),
			FREQ2FBIN(2417, 1),
			FREQ2FBIN(2457, 1),
			FREQ2FBIN(2462, 1)
		},
		{
			FREQ2FBIN(2412, 1),
			FREQ2FBIN(2417, 1),
			FREQ2FBIN(2462, 1),
			0xFF,
		},

		{
			FREQ2FBIN(2412, 1),
			FREQ2FBIN(2417, 1),
			FREQ2FBIN(2462, 1),
			0xFF,
		},
		{
			FREQ2FBIN(2422, 1),
			FREQ2FBIN(2427, 1),
			FREQ2FBIN(2447, 1),
			FREQ2FBIN(2452, 1)
		},

		{
			/* Data[4].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[4].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			/* Data[4].ctlEdges[2].bChannel */ FREQ2FBIN(2472, 1),
			/* Data[4].ctlEdges[3].bChannel */ FREQ2FBIN(2484, 1),
		},

		{
			/* Data[5].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[5].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			/* Data[5].ctlEdges[2].bChannel */ FREQ2FBIN(2472, 1),
			0,
		},

		{
			/* Data[6].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[6].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			FREQ2FBIN(2472, 1),
			0,
		},

		{
			/* Data[7].ctlEdges[0].bChannel */ FREQ2FBIN(2422, 1),
			/* Data[7].ctlEdges[1].bChannel */ FREQ2FBIN(2427, 1),
			/* Data[7].ctlEdges[2].bChannel */ FREQ2FBIN(2447, 1),
			/* Data[7].ctlEdges[3].bChannel */ FREQ2FBIN(2462, 1),
		},

		{
			/* Data[8].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[8].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			/* Data[8].ctlEdges[2].bChannel */ FREQ2FBIN(2472, 1),
		},

		{
			/* Data[9].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[9].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			/* Data[9].ctlEdges[2].bChannel */ FREQ2FBIN(2472, 1),
			0
		},

		{
			/* Data[10].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[10].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			/* Data[10].ctlEdges[2].bChannel */ FREQ2FBIN(2472, 1),
			0
		},

		{
			/* Data[11].ctlEdges[0].bChannel */ FREQ2FBIN(2422, 1),
			/* Data[11].ctlEdges[1].bChannel */ FREQ2FBIN(2427, 1),
			/* Data[11].ctlEdges[2].bChannel */ FREQ2FBIN(2447, 1),
			/* Data[11].ctlEdges[3].bChannel */ FREQ2FBIN(2462, 1),
		}
	},
	.ctlPowerData_2G = {
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 1) } },

		{ { CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },

		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },

		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1) } }
	},
	.modalHeader5G = {
		.antCtrlCommon = LE32CONST(0x00000220),
		.antCtrlCommon2 = LE32CONST(0x00011111),
		.antCtrlChain = {
			LE16CONST(0x0150), LE16CONST(0x0150), LE16CONST(0x0150)
		},
		/* 4 idle,t1,t2,b (4 bits per setting) */
		.xatten1DB = {0, 0, 0},

		/*
		 * xatten1Margin[AR9300_MAX_CHAINS]; 3 xatten1_margin
		 * for merlin (0xa20c/b20c 16:12
		 */
		.xatten1Margin = {0, 0, 0},
		.tempSlope = 68,
		.voltSlope = 0,
		/* spurChans spur channels in usual fbin coding format */
		.spurChans = {FREQ2FBIN(5500, 0), 0, 0, 0, 0},
		/* noiseFloorThreshCh Check if the register is per chain */
		.noiseFloorThreshCh = {-1, 0, 0},
		.reserved = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.quick_drop = 0,
		.xpaBiasLvl = 0xf,
		.txFrameToDataStart = 0x0e,
		.txFrameToPaOn = 0x0e,
		.txClip = 3, /* 4 bits tx_clip, 4 bits dac_scale_cck */
		.antennaGain = 0,
		.switchSettling = 0x2d,
		.adcDesiredSize = -30,
		.txEndToXpaOff = 0,
		.txEndToRxOn = 0x2,
		.txFrameToXpaOn = 0xe,
		.thresh62 = 28,
		.xlna_bias_strength = 0,
		.futureModal = {
			0, 0, 0, 0, 0, 0, 0,
		},
	},
	.base_ext2 = {
		.tempSlopeLow = 72,
		.tempSlopeHigh = 105,
		.xatten1DBLow = {0, 0, 0},
		.xatten1MarginLow = {0, 0, 0},
		.xatten1DBHigh = {0, 0, 0},
		.xatten1MarginHigh = {0, 0, 0}
	},
	.calFreqPier5G = {
		FREQ2FBIN(5180, 0),
		FREQ2FBIN(5240, 0),
		FREQ2FBIN(5320, 0),
		FREQ2FBIN(5400, 0),
		FREQ2FBIN(5500, 0),
		FREQ2FBIN(5600, 0),
		FREQ2FBIN(5745, 0),
		FREQ2FBIN(5785, 0)
	},
	.calPierData5G = {
			{
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
			},
			{
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
			},
			{
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
			},

	},
	.calTarget_freqbin_5G = {
		FREQ2FBIN(5180, 0),
		FREQ2FBIN(5220, 0),
		FREQ2FBIN(5320, 0),
		FREQ2FBIN(5400, 0),
		FREQ2FBIN(5500, 0),
		FREQ2FBIN(5600, 0),
		FREQ2FBIN(5745, 0),
		FREQ2FBIN(5785, 0)
	},
	.calTarget_freqbin_5GHT20 = {
		FREQ2FBIN(5180, 0),
		FREQ2FBIN(5240, 0),
		FREQ2FBIN(5320, 0),
		FREQ2FBIN(5400, 0),
		FREQ2FBIN(5500, 0),
		FREQ2FBIN(5700, 0),
		FREQ2FBIN(5745, 0),
		FREQ2FBIN(5825, 0)
	},
	.calTarget_freqbin_5GHT40 = {
		FREQ2FBIN(5190, 0),
		FREQ2FBIN(5230, 0),
		FREQ2FBIN(5320, 0),
		FREQ2FBIN(5410, 0),
		FREQ2FBIN(5510, 0),
		FREQ2FBIN(5670, 0),
		FREQ2FBIN(5755, 0),
		FREQ2FBIN(5825, 0)
	},
	.calTargetPower5G = {
		/* 6-24,36,48,54 */
		{ { PWR2X(21), PWR2X(20), PWR2X(20), PWR2X(17) } },
		{ { PWR2X(21), PWR2X(20), PWR2X(20), PWR2X(17) } },
		{ { PWR2X(21), PWR2X(20), PWR2X(20), PWR2X(17) } },
		{ { PWR2X(21), PWR2X(20), PWR2X(20), PWR2X(17) } },
		{ { PWR2X(21), PWR2X(20), PWR2X(20), PWR2X(17) } },
		{ { PWR2X(21), PWR2X(20), PWR2X(20), PWR2X(17) } },
		{ { PWR2X(21), PWR2X(20), PWR2X(20), PWR2X(17) } },
		{ { PWR2X(21), PWR2X(20), PWR2X(20), PWR2X(17) } }
	},
	.calTargetPower5GHT20 = {
		/*
		 * 0_8_16,1-3_9-11_17-19,
		 * 4,5,6,7,
		 * 12,13,14,15,
		 * 20,21,22,23
		 */
		{{
			PWR2X(20), PWR2X(20),
			PWR2X(20), PWR2X(20), PWR2X(16), PWR2X(14),
			PWR2X(20), PWR2X(20), PWR2X(16), PWR2X(14),
			PWR2X(20), PWR2X(20), PWR2X(16), PWR2X(10)
		}}, {{
			PWR2X(20), PWR2X(20),
			PWR2X(20), PWR2X(20), PWR2X(16), PWR2X(14),
			PWR2X(20), PWR2X(20), PWR2X(16), PWR2X(14),
			PWR2X(20), PWR2X(20), PWR2X(16), PWR2X(10)
		}}, {{
			PWR2X(20), PWR2X(20),
			PWR2X(20), PWR2X(20), PWR2X(16), PWR2X(14),
			PWR2X(20), PWR2X(20), PWR2X(16), PWR2X(14),
			PWR2X(20), PWR2X(20), PWR2X(16), PWR2X(10)
		}}, {{
			PWR2X(20), PWR2X(20),
			PWR2X(20), PWR2X(20), PWR2X(16), PWR2X(14),
			PWR2X(20), PWR2X(20), PWR2X(16), PWR2X(14),
			PWR2X(20), PWR2X(20), PWR2X(16), PWR2X(10)
		}}, {{
			PWR2X(20), PWR2X(20),
			PWR2X(20), PWR2X(20), PWR2X(16), PWR2X(14),
			PWR2X(20), PWR2X(20), PWR2X(16), PWR2X(14),
			PWR2X(20), PWR2X(20), PWR2X(16), PWR2X(10)
		}}, {{
			PWR2X(20), PWR2X(20),
			PWR2X(20), PWR2X(20), PWR2X(16), PWR2X(14),
			PWR2X(20), PWR2X(20), PWR2X(16), PWR2X(14),
			PWR2X(20), PWR2X(20), PWR2X(16), PWR2X(10)
		}}, {{
			PWR2X(19), PWR2X(19),
			PWR2X(19), PWR2X(19), PWR2X(16), PWR2X(14),
			PWR2X(19), PWR2X(19), PWR2X(16), PWR2X(14),
			PWR2X(19), PWR2X(19), PWR2X(16), PWR2X(13)
		}}, {{
			PWR2X(18), PWR2X(18),
			PWR2X(18), PWR2X(18), PWR2X(16), PWR2X(14),
			PWR2X(18), PWR2X(18), PWR2X(16), PWR2X(14),
			PWR2X(18), PWR2X(18), PWR2X(16), PWR2X(13)
		}}
	},
	.calTargetPower5GHT40 =  {
		/*
		 * 0_8_16,1-3_9-11_17-19,
		 * 4,5,6,7,
		 * 12,13,14,15,
		 * 20,21,22,23
		 */
		{{
			PWR2X(20), PWR2X(20),
			PWR2X(20), PWR2X(19), PWR2X(15), PWR2X(13),
			PWR2X(20), PWR2X(20), PWR2X(15), PWR2X(13),
			PWR2X(20), PWR2X(20), PWR2X(15), PWR2X(12)
		}}, {{
			PWR2X(20), PWR2X(20),
			PWR2X(20), PWR2X(19), PWR2X(15), PWR2X(13),
			PWR2X(20), PWR2X(20), PWR2X(15), PWR2X(13),
			PWR2X(20), PWR2X(20), PWR2X(15), PWR2X(12)
		}}, {{
			PWR2X(20), PWR2X(20),
			PWR2X(20), PWR2X(19), PWR2X(15), PWR2X(13),
			PWR2X(20), PWR2X(20), PWR2X(15), PWR2X(13),
			PWR2X(20), PWR2X(20), PWR2X(15), PWR2X(12)
		}}, {{
			PWR2X(20), PWR2X(20),
			PWR2X(20), PWR2X(19), PWR2X(15), PWR2X(13),
			PWR2X(20), PWR2X(20), PWR2X(15), PWR2X(13),
			PWR2X(20), PWR2X(20), PWR2X(15), PWR2X(12)
		}}, {{
			PWR2X(20), PWR2X(20),
			PWR2X(20), PWR2X(19), PWR2X(15), PWR2X(13),
			PWR2X(20), PWR2X(20), PWR2X(15), PWR2X(13),
			PWR2X(20), PWR2X(20), PWR2X(15), PWR2X(12)
		}}, {{
			PWR2X(20), PWR2X(20),
			PWR2X(20), PWR2X(19), PWR2X(15), PWR2X(13),
			PWR2X(20), PWR2X(20), PWR2X(15), PWR2X(13),
			PWR2X(20), PWR2X(20), PWR2X(15), PWR2X(12)
		}}, {{
			PWR2X(18), PWR2X(18),
			PWR2X(18), PWR2X(18), PWR2X(15), PWR2X(13),
			PWR2X(18), PWR2X(18), PWR2X(15), PWR2X(13),
			PWR2X(18), PWR2X(18), PWR2X(15), PWR2X(12)
		}}, {{
			PWR2X(17), PWR2X(17),
			PWR2X(17), PWR2X(17), PWR2X(15), PWR2X(13),
			PWR2X(17), PWR2X(17), PWR2X(15), PWR2X(13),
			PWR2X(17), PWR2X(17), PWR2X(15), PWR2X(12)
		}}
	},
	.ctlIndex_5G =  {
		0x10, 0x16, 0x18, 0x40, 0x46,
		0x48, 0x30, 0x36, 0x38
	},
	.ctl_freqbin_5G =  {
		{
			/* Data[0].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[0].ctlEdges[1].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[0].ctlEdges[2].bChannel */ FREQ2FBIN(5280, 0),
			/* Data[0].ctlEdges[3].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[0].ctlEdges[4].bChannel */ FREQ2FBIN(5600, 0),
			/* Data[0].ctlEdges[5].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[0].ctlEdges[6].bChannel */ FREQ2FBIN(5745, 0),
			/* Data[0].ctlEdges[7].bChannel */ FREQ2FBIN(5825, 0)
		},
		{
			/* Data[1].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[1].ctlEdges[1].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[1].ctlEdges[2].bChannel */ FREQ2FBIN(5280, 0),
			/* Data[1].ctlEdges[3].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[1].ctlEdges[4].bChannel */ FREQ2FBIN(5520, 0),
			/* Data[1].ctlEdges[5].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[1].ctlEdges[6].bChannel */ FREQ2FBIN(5745, 0),
			/* Data[1].ctlEdges[7].bChannel */ FREQ2FBIN(5825, 0)
		},

		{
			/* Data[2].ctlEdges[0].bChannel */ FREQ2FBIN(5190, 0),
			/* Data[2].ctlEdges[1].bChannel */ FREQ2FBIN(5230, 0),
			/* Data[2].ctlEdges[2].bChannel */ FREQ2FBIN(5270, 0),
			/* Data[2].ctlEdges[3].bChannel */ FREQ2FBIN(5310, 0),
			/* Data[2].ctlEdges[4].bChannel */ FREQ2FBIN(5510, 0),
			/* Data[2].ctlEdges[5].bChannel */ FREQ2FBIN(5550, 0),
			/* Data[2].ctlEdges[6].bChannel */ FREQ2FBIN(5670, 0),
			/* Data[2].ctlEdges[7].bChannel */ FREQ2FBIN(5755, 0)
		},

		{
			/* Data[3].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[3].ctlEdges[1].bChannel */ FREQ2FBIN(5200, 0),
			/* Data[3].ctlEdges[2].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[3].ctlEdges[3].bChannel */ FREQ2FBIN(5320, 0),
			/* Data[3].ctlEdges[4].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[3].ctlEdges[5].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[3].ctlEdges[6].bChannel */ 0xFF,
			/* Data[3].ctlEdges[7].bChannel */ 0xFF,
		},

		{
			/* Data[4].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[4].ctlEdges[1].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[4].ctlEdges[2].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[4].ctlEdges[3].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[4].ctlEdges[4].bChannel */ 0xFF,
			/* Data[4].ctlEdges[5].bChannel */ 0xFF,
			/* Data[4].ctlEdges[6].bChannel */ 0xFF,
			/* Data[4].ctlEdges[7].bChannel */ 0xFF,
		},

		{
			/* Data[5].ctlEdges[0].bChannel */ FREQ2FBIN(5190, 0),
			/* Data[5].ctlEdges[1].bChannel */ FREQ2FBIN(5270, 0),
			/* Data[5].ctlEdges[2].bChannel */ FREQ2FBIN(5310, 0),
			/* Data[5].ctlEdges[3].bChannel */ FREQ2FBIN(5510, 0),
			/* Data[5].ctlEdges[4].bChannel */ FREQ2FBIN(5590, 0),
			/* Data[5].ctlEdges[5].bChannel */ FREQ2FBIN(5670, 0),
			/* Data[5].ctlEdges[6].bChannel */ 0xFF,
			/* Data[5].ctlEdges[7].bChannel */ 0xFF
		},

		{
			/* Data[6].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[6].ctlEdges[1].bChannel */ FREQ2FBIN(5200, 0),
			/* Data[6].ctlEdges[2].bChannel */ FREQ2FBIN(5220, 0),
			/* Data[6].ctlEdges[3].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[6].ctlEdges[4].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[6].ctlEdges[5].bChannel */ FREQ2FBIN(5600, 0),
			/* Data[6].ctlEdges[6].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[6].ctlEdges[7].bChannel */ FREQ2FBIN(5745, 0)
		},

		{
			/* Data[7].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[7].ctlEdges[1].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[7].ctlEdges[2].bChannel */ FREQ2FBIN(5320, 0),
			/* Data[7].ctlEdges[3].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[7].ctlEdges[4].bChannel */ FREQ2FBIN(5560, 0),
			/* Data[7].ctlEdges[5].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[7].ctlEdges[6].bChannel */ FREQ2FBIN(5745, 0),
			/* Data[7].ctlEdges[7].bChannel */ FREQ2FBIN(5825, 0)
		},

		{
			/* Data[8].ctlEdges[0].bChannel */ FREQ2FBIN(5190, 0),
			/* Data[8].ctlEdges[1].bChannel */ FREQ2FBIN(5230, 0),
			/* Data[8].ctlEdges[2].bChannel */ FREQ2FBIN(5270, 0),
			/* Data[8].ctlEdges[3].bChannel */ FREQ2FBIN(5510, 0),
			/* Data[8].ctlEdges[4].bChannel */ FREQ2FBIN(5550, 0),
			/* Data[8].ctlEdges[5].bChannel */ FREQ2FBIN(5670, 0),
			/* Data[8].ctlEdges[6].bChannel */ FREQ2FBIN(5755, 0),
			/* Data[8].ctlEdges[7].bChannel */ FREQ2FBIN(5795, 0)
		}
	},
	.ctlPowerData_5G = {
		{{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1)
		}}, {{
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0),
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0),
			CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1)
		}}
	}
};


static const struct ar9300_eeprom ar9300_h112 = {
	.eepromVersion = 2,
	.templateVersion = ar9300_tpl_ver_h112,
	.macAddr = {0x00, 0x03, 0x7f, 0x0, 0x0, 0x0},
	.custData = {"h112-241-f0000"},
	.baseEepHeader = {
		.regDmn = { LE16CONST(0x0000), LE16CONST(0x001f) },
		.txrxMask =  0x77, /* 4 bits tx and 4 bits rx */
		.opCapFlags = {
			.opFlags = AR5416_OPFLAGS_11G | AR5416_OPFLAGS_11A,
			.eepMisc = 0,
		},
		.rfSilent = 0,
		.blueToothOptions = 0,
		.deviceCap = 0,
		.deviceType = 5, /* takes lower byte in eeprom location */
		.pwrTableOffset = AR9300_PWR_TABLE_OFFSET,
		.params_for_tuning_caps = {0, 0},
		.featureEnable = 0x0d,
		/*
		 * bit0 - enable tx temp comp - disabled
		 * bit1 - enable tx volt comp - disabled
		 * bit2 - enable fastClock - enabled
		 * bit3 - enable doubling - enabled
		 * bit4 - enable internal regulator - disabled
		 * bit5 - enable pa predistortion - disabled
		 */
		.miscConfiguration = 0, /* bit0 - turn down drivestrength */
		.eepromWriteEnableGpio = 6,
		.wlanDisableGpio = 0,
		.wlanLedGpio = 8,
		.rxBandSelectGpio = 0xff,
		.txrxgain = 0x10,
		.swreg = 0,
	},
	.modalHeader2G = {
		/* ar9300_modal_eep_header  2g */
		.antCtrlCommon = LE32CONST(0x00000110),
		.antCtrlCommon2 = LE32CONST(0x00044444),
		.antCtrlChain = {
			LE16CONST(0x0150), LE16CONST(0x0150), LE16CONST(0x0150)
		},
		/*
		 * xatten1DB[AR9300_MAX_CHAINS];  3 xatten1_db
		 * for ar9280 (0xa20c/b20c 5:0)
		 */
		.xatten1DB = {0, 0, 0},

		/*
		 * xatten1Margin[AR9300_MAX_CHAINS]; 3 xatten1_margin
		 * for ar9280 (0xa20c/b20c 16:12
		 */
		.xatten1Margin = {0, 0, 0},
		.tempSlope = 25,
		.voltSlope = 0,

		/*
		 * spurChans[OSPREY_EEPROM_MODAL_SPURS]; spur
		 * channels in usual fbin coding format
		 */
		.spurChans = {FREQ2FBIN(2464, 1), 0, 0, 0, 0},

		/*
		 * noiseFloorThreshCh[AR9300_MAX_CHAINS]; 3 Check
		 * if the register is per chain
		 */
		.noiseFloorThreshCh = {-1, 0, 0},
		.reserved = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.quick_drop = 0,
		.xpaBiasLvl = 0,
		.txFrameToDataStart = 0x0e,
		.txFrameToPaOn = 0x0e,
		.txClip = 3, /* 4 bits tx_clip, 4 bits dac_scale_cck */
		.antennaGain = 0,
		.switchSettling = 0x2c,
		.adcDesiredSize = -30,
		.txEndToXpaOff = 0,
		.txEndToRxOn = 0x2,
		.txFrameToXpaOn = 0xe,
		.thresh62 = 28,
		.xlna_bias_strength = 0,
		.futureModal = {
			0, 0, 0, 0, 0, 0, 0,
		},
	},
	.base_ext1 = {
		.ant_div_control = 0,
		.future = {0, 0, 0},
		.tempslopextension = {0, 0, 0, 0, 0, 0, 0, 0}
	},
	.calFreqPier2G = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2462, 1),
	},
	/* ar9300_cal_data_per_freq_op_loop 2g */
	.calPierData2G = {
		{ {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0} },
		{ {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0} },
		{ {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0} },
	},
	.calTarget_freqbin_Cck = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2472, 1),
	},
	.calTarget_freqbin_2G = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2472, 1)
	},
	.calTarget_freqbin_2GHT20 = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2472, 1)
	},
	.calTarget_freqbin_2GHT40 = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2472, 1)
	},
	.calTargetPowerCck = {
		/* 1L-5L,5S,11L,11S */
		{ { PWR2X(17), PWR2X(17), PWR2X(17), PWR2X(17) } },
		{ { PWR2X(17), PWR2X(17), PWR2X(17), PWR2X(17) } }
	},
	.calTargetPower2G = {
		/* 6-24,36,48,54 */
		{ { PWR2X(17), PWR2X(17), PWR2X(16), PWR2X(16) } },
		{ { PWR2X(17), PWR2X(17), PWR2X(16), PWR2X(16) } },
		{ { PWR2X(17), PWR2X(17), PWR2X(16), PWR2X(16) } }
	},
	.calTargetPower2GHT20 = {
		{{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(16), PWR2X(16), PWR2X(15),
			PWR2X(16), PWR2X(16), PWR2X(15), PWR2X(14),
			PWR2X(14), PWR2X(14), PWR2X(14), PWR2X(12)
		}}, {{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(16), PWR2X(16), PWR2X(15),
			PWR2X(16), PWR2X(16), PWR2X(15), PWR2X(14),
			PWR2X(14), PWR2X(14), PWR2X(14), PWR2X(12)
		}}, {{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(16), PWR2X(16), PWR2X(15),
			PWR2X(16), PWR2X(16), PWR2X(15), PWR2X(14),
			PWR2X(14), PWR2X(14), PWR2X(14), PWR2X(12)
		}}
	},
	.calTargetPower2GHT40 = {
		{{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(15), PWR2X(15), PWR2X(14),
			PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(13),
			PWR2X(13), PWR2X(13), PWR2X(13), PWR2X(11)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(15), PWR2X(15), PWR2X(14),
			PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(13),
			PWR2X(13), PWR2X(13), PWR2X(13), PWR2X(11)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(15), PWR2X(15), PWR2X(14),
			PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(13),
			PWR2X(13), PWR2X(13), PWR2X(13), PWR2X(11)
		}}
	},
	.ctlIndex_2G =  {
		0x11, 0x12, 0x15, 0x17, 0x41, 0x42,
		0x45, 0x47, 0x31, 0x32, 0x35, 0x37,
	},
	.ctl_freqbin_2G = {
		{
			FREQ2FBIN(2412, 1),
			FREQ2FBIN(2417, 1),
			FREQ2FBIN(2457, 1),
			FREQ2FBIN(2462, 1)
		},
		{
			FREQ2FBIN(2412, 1),
			FREQ2FBIN(2417, 1),
			FREQ2FBIN(2462, 1),
			0xFF,
		},

		{
			FREQ2FBIN(2412, 1),
			FREQ2FBIN(2417, 1),
			FREQ2FBIN(2462, 1),
			0xFF,
		},
		{
			FREQ2FBIN(2422, 1),
			FREQ2FBIN(2427, 1),
			FREQ2FBIN(2447, 1),
			FREQ2FBIN(2452, 1)
		},

		{
			/* Data[4].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[4].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			/* Data[4].ctlEdges[2].bChannel */ FREQ2FBIN(2472, 1),
			/* Data[4].ctlEdges[3].bChannel */ FREQ2FBIN(2484, 1),
		},

		{
			/* Data[5].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[5].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			/* Data[5].ctlEdges[2].bChannel */ FREQ2FBIN(2472, 1),
			0,
		},

		{
			/* Data[6].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[6].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			FREQ2FBIN(2472, 1),
			0,
		},

		{
			/* Data[7].ctlEdges[0].bChannel */ FREQ2FBIN(2422, 1),
			/* Data[7].ctlEdges[1].bChannel */ FREQ2FBIN(2427, 1),
			/* Data[7].ctlEdges[2].bChannel */ FREQ2FBIN(2447, 1),
			/* Data[7].ctlEdges[3].bChannel */ FREQ2FBIN(2462, 1),
		},

		{
			/* Data[8].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[8].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			/* Data[8].ctlEdges[2].bChannel */ FREQ2FBIN(2472, 1),
		},

		{
			/* Data[9].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[9].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			/* Data[9].ctlEdges[2].bChannel */ FREQ2FBIN(2472, 1),
			0
		},

		{
			/* Data[10].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[10].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			/* Data[10].ctlEdges[2].bChannel */ FREQ2FBIN(2472, 1),
			0
		},

		{
			/* Data[11].ctlEdges[0].bChannel */ FREQ2FBIN(2422, 1),
			/* Data[11].ctlEdges[1].bChannel */ FREQ2FBIN(2427, 1),
			/* Data[11].ctlEdges[2].bChannel */ FREQ2FBIN(2447, 1),
			/* Data[11].ctlEdges[3].bChannel */ FREQ2FBIN(2462, 1),
		}
	},
	.ctlPowerData_2G = {
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 1) } },

		{ { CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },

		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },

		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1) } }
	},
	.modalHeader5G = {
		.antCtrlCommon = LE32CONST(0x00000220),
		.antCtrlCommon2 = LE32CONST(0x00044444),
		.antCtrlChain = {
			LE16CONST(0x0150), LE16CONST(0x0150), LE16CONST(0x0150)
		},
		/* xatten1DB 3 xatten1_db for AR9280 (0xa20c/b20c 5:0) */
		.xatten1DB = {0, 0, 0},

		/*
		 * xatten1Margin[AR9300_MAX_CHAINS]; 3 xatten1_margin
		 * for merlin (0xa20c/b20c 16:12
		 */
		.xatten1Margin = {0, 0, 0},
		.tempSlope = 45,
		.voltSlope = 0,
		/* spurChans spur channels in usual fbin coding format */
		.spurChans = {0, 0, 0, 0, 0},
		/* noiseFloorThreshCh Check if the register is per chain */
		.noiseFloorThreshCh = {-1, 0, 0},
		.reserved = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.quick_drop = 0,
		.xpaBiasLvl = 0,
		.txFrameToDataStart = 0x0e,
		.txFrameToPaOn = 0x0e,
		.txClip = 3, /* 4 bits tx_clip, 4 bits dac_scale_cck */
		.antennaGain = 0,
		.switchSettling = 0x2d,
		.adcDesiredSize = -30,
		.txEndToXpaOff = 0,
		.txEndToRxOn = 0x2,
		.txFrameToXpaOn = 0xe,
		.thresh62 = 28,
		.xlna_bias_strength = 0,
		.futureModal = {
			0, 0, 0, 0, 0, 0, 0,
		},
	},
	.base_ext2 = {
		.tempSlopeLow = 40,
		.tempSlopeHigh = 50,
		.xatten1DBLow = {0, 0, 0},
		.xatten1MarginLow = {0, 0, 0},
		.xatten1DBHigh = {0, 0, 0},
		.xatten1MarginHigh = {0, 0, 0}
	},
	.calFreqPier5G = {
		FREQ2FBIN(5180, 0),
		FREQ2FBIN(5220, 0),
		FREQ2FBIN(5320, 0),
		FREQ2FBIN(5400, 0),
		FREQ2FBIN(5500, 0),
		FREQ2FBIN(5600, 0),
		FREQ2FBIN(5700, 0),
		FREQ2FBIN(5785, 0)
	},
	.calPierData5G = {
		{
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
		},
		{
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
		},
		{
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
		},

	},
	.calTarget_freqbin_5G = {
		FREQ2FBIN(5180, 0),
		FREQ2FBIN(5240, 0),
		FREQ2FBIN(5320, 0),
		FREQ2FBIN(5400, 0),
		FREQ2FBIN(5500, 0),
		FREQ2FBIN(5600, 0),
		FREQ2FBIN(5700, 0),
		FREQ2FBIN(5825, 0)
	},
	.calTarget_freqbin_5GHT20 = {
		FREQ2FBIN(5180, 0),
		FREQ2FBIN(5240, 0),
		FREQ2FBIN(5320, 0),
		FREQ2FBIN(5400, 0),
		FREQ2FBIN(5500, 0),
		FREQ2FBIN(5700, 0),
		FREQ2FBIN(5745, 0),
		FREQ2FBIN(5825, 0)
	},
	.calTarget_freqbin_5GHT40 = {
		FREQ2FBIN(5180, 0),
		FREQ2FBIN(5240, 0),
		FREQ2FBIN(5320, 0),
		FREQ2FBIN(5400, 0),
		FREQ2FBIN(5500, 0),
		FREQ2FBIN(5700, 0),
		FREQ2FBIN(5745, 0),
		FREQ2FBIN(5825, 0)
	},
	.calTargetPower5G = {
		/* 6-24,36,48,54 */
		{ { PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(12) } },
		{ { PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(12) } },
		{ { PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(12) } },
		{ { PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(12) } },
		{ { PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(12) } },
		{ { PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(12) } },
		{ { PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(12) } },
		{ { PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(12) } }
	},
	.calTargetPower5GHT20 = {
		/*
		 * 0_8_16,1-3_9-11_17-19,
		 * 4,5,6,7,
		 * 12,13,14,15,
		 * 20,21,22,23
		 */
		{{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(14), PWR2X(12), PWR2X(10),
			PWR2X(15), PWR2X(14), PWR2X(12), PWR2X(10),
			PWR2X(10), PWR2X(10), PWR2X(10), PWR2X(8)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(14), PWR2X(12), PWR2X(10),
			PWR2X(15), PWR2X(14), PWR2X(12), PWR2X(10),
			PWR2X(10), PWR2X(10), PWR2X(10), PWR2X(8)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(13), PWR2X(11), PWR2X(9),
			PWR2X(15), PWR2X(13), PWR2X(11), PWR2X(9),
			PWR2X(9), PWR2X(9), PWR2X(9), PWR2X(8)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(13), PWR2X(11), PWR2X(9),
			PWR2X(15), PWR2X(13), PWR2X(11), PWR2X(9),
			PWR2X(9), PWR2X(9), PWR2X(9), PWR2X(8)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(12), PWR2X(10), PWR2X(8),
			PWR2X(15), PWR2X(12), PWR2X(10), PWR2X(8),
			PWR2X(8), PWR2X(8), PWR2X(8), PWR2X(7)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(12), PWR2X(10), PWR2X(8),
			PWR2X(15), PWR2X(12), PWR2X(10), PWR2X(8),
			PWR2X(8), PWR2X(8), PWR2X(8), PWR2X(7)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(11), PWR2X(9), PWR2X(7),
			PWR2X(15), PWR2X(11), PWR2X(9), PWR2X(7),
			PWR2X(7), PWR2X(7), PWR2X(7), PWR2X(6)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(11), PWR2X(9), PWR2X(7),
			PWR2X(15), PWR2X(11), PWR2X(9), PWR2X(7),
			PWR2X(7), PWR2X(7), PWR2X(7), PWR2X(6)
		}}
	},
	.calTargetPower5GHT40 =  {
		/*
		 * 0_8_16,1-3_9-11_17-19,
		 * 4,5,6,7,
		 * 12,13,14,15,
		 * 20,21,22,23
		 */
		{{
			PWR2X(14), PWR2X(14),
			PWR2X(14), PWR2X(13), PWR2X(11), PWR2X(9),
			PWR2X(14), PWR2X(13), PWR2X(11), PWR2X(9),
			PWR2X(9), PWR2X(9), PWR2X(9), PWR2X(7)
		}}, {{
			PWR2X(14), PWR2X(14),
			PWR2X(14), PWR2X(13), PWR2X(11), PWR2X(9),
			PWR2X(14), PWR2X(13), PWR2X(11), PWR2X(9),
			PWR2X(9), PWR2X(9), PWR2X(9), PWR2X(7)
		}}, {{
			PWR2X(14), PWR2X(14),
			PWR2X(14), PWR2X(12), PWR2X(10), PWR2X(8),
			PWR2X(14), PWR2X(12), PWR2X(10), PWR2X(8),
			PWR2X(8), PWR2X(8), PWR2X(8), PWR2X(6)
		}}, {{
			PWR2X(14), PWR2X(14),
			PWR2X(14), PWR2X(12), PWR2X(10), PWR2X(8),
			PWR2X(14), PWR2X(12), PWR2X(10), PWR2X(8),
			PWR2X(8), PWR2X(8), PWR2X(8), PWR2X(6)
		}}, {{
			PWR2X(14), PWR2X(14),
			PWR2X(14), PWR2X(11), PWR2X(9), PWR2X(7),
			PWR2X(14), PWR2X(11), PWR2X(9), PWR2X(7),
			PWR2X(7), PWR2X(7), PWR2X(7), PWR2X(5)
		}}, {{
			PWR2X(14), PWR2X(14),
			PWR2X(14), PWR2X(11), PWR2X(9), PWR2X(7),
			PWR2X(14), PWR2X(11), PWR2X(9), PWR2X(7),
			PWR2X(7), PWR2X(7), PWR2X(7), PWR2X(5)
		}}, {{
			PWR2X(14), PWR2X(14),
			PWR2X(14), PWR2X(10), PWR2X(8), PWR2X(6),
			PWR2X(14), PWR2X(10), PWR2X(8), PWR2X(6),
			PWR2X(6), PWR2X(6), PWR2X(6), PWR2X(4)
		}}, {{
			PWR2X(14), PWR2X(14),
			PWR2X(14), PWR2X(10), PWR2X(8), PWR2X(6),
			PWR2X(14), PWR2X(10), PWR2X(8), PWR2X(6),
			PWR2X(6), PWR2X(6), PWR2X(6), PWR2X(4)
		}}
	},
	.ctlIndex_5G =  {
		0x10, 0x16, 0x18, 0x40, 0x46,
		0x48, 0x30, 0x36, 0x38
	},
	.ctl_freqbin_5G =  {
		{
			/* Data[0].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[0].ctlEdges[1].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[0].ctlEdges[2].bChannel */ FREQ2FBIN(5280, 0),
			/* Data[0].ctlEdges[3].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[0].ctlEdges[4].bChannel */ FREQ2FBIN(5600, 0),
			/* Data[0].ctlEdges[5].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[0].ctlEdges[6].bChannel */ FREQ2FBIN(5745, 0),
			/* Data[0].ctlEdges[7].bChannel */ FREQ2FBIN(5825, 0)
		},
		{
			/* Data[1].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[1].ctlEdges[1].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[1].ctlEdges[2].bChannel */ FREQ2FBIN(5280, 0),
			/* Data[1].ctlEdges[3].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[1].ctlEdges[4].bChannel */ FREQ2FBIN(5520, 0),
			/* Data[1].ctlEdges[5].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[1].ctlEdges[6].bChannel */ FREQ2FBIN(5745, 0),
			/* Data[1].ctlEdges[7].bChannel */ FREQ2FBIN(5825, 0)
		},

		{
			/* Data[2].ctlEdges[0].bChannel */ FREQ2FBIN(5190, 0),
			/* Data[2].ctlEdges[1].bChannel */ FREQ2FBIN(5230, 0),
			/* Data[2].ctlEdges[2].bChannel */ FREQ2FBIN(5270, 0),
			/* Data[2].ctlEdges[3].bChannel */ FREQ2FBIN(5310, 0),
			/* Data[2].ctlEdges[4].bChannel */ FREQ2FBIN(5510, 0),
			/* Data[2].ctlEdges[5].bChannel */ FREQ2FBIN(5550, 0),
			/* Data[2].ctlEdges[6].bChannel */ FREQ2FBIN(5670, 0),
			/* Data[2].ctlEdges[7].bChannel */ FREQ2FBIN(5755, 0)
		},

		{
			/* Data[3].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[3].ctlEdges[1].bChannel */ FREQ2FBIN(5200, 0),
			/* Data[3].ctlEdges[2].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[3].ctlEdges[3].bChannel */ FREQ2FBIN(5320, 0),
			/* Data[3].ctlEdges[4].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[3].ctlEdges[5].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[3].ctlEdges[6].bChannel */ 0xFF,
			/* Data[3].ctlEdges[7].bChannel */ 0xFF,
		},

		{
			/* Data[4].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[4].ctlEdges[1].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[4].ctlEdges[2].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[4].ctlEdges[3].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[4].ctlEdges[4].bChannel */ 0xFF,
			/* Data[4].ctlEdges[5].bChannel */ 0xFF,
			/* Data[4].ctlEdges[6].bChannel */ 0xFF,
			/* Data[4].ctlEdges[7].bChannel */ 0xFF,
		},

		{
			/* Data[5].ctlEdges[0].bChannel */ FREQ2FBIN(5190, 0),
			/* Data[5].ctlEdges[1].bChannel */ FREQ2FBIN(5270, 0),
			/* Data[5].ctlEdges[2].bChannel */ FREQ2FBIN(5310, 0),
			/* Data[5].ctlEdges[3].bChannel */ FREQ2FBIN(5510, 0),
			/* Data[5].ctlEdges[4].bChannel */ FREQ2FBIN(5590, 0),
			/* Data[5].ctlEdges[5].bChannel */ FREQ2FBIN(5670, 0),
			/* Data[5].ctlEdges[6].bChannel */ 0xFF,
			/* Data[5].ctlEdges[7].bChannel */ 0xFF
		},

		{
			/* Data[6].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[6].ctlEdges[1].bChannel */ FREQ2FBIN(5200, 0),
			/* Data[6].ctlEdges[2].bChannel */ FREQ2FBIN(5220, 0),
			/* Data[6].ctlEdges[3].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[6].ctlEdges[4].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[6].ctlEdges[5].bChannel */ FREQ2FBIN(5600, 0),
			/* Data[6].ctlEdges[6].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[6].ctlEdges[7].bChannel */ FREQ2FBIN(5745, 0)
		},

		{
			/* Data[7].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[7].ctlEdges[1].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[7].ctlEdges[2].bChannel */ FREQ2FBIN(5320, 0),
			/* Data[7].ctlEdges[3].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[7].ctlEdges[4].bChannel */ FREQ2FBIN(5560, 0),
			/* Data[7].ctlEdges[5].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[7].ctlEdges[6].bChannel */ FREQ2FBIN(5745, 0),
			/* Data[7].ctlEdges[7].bChannel */ FREQ2FBIN(5825, 0)
		},

		{
			/* Data[8].ctlEdges[0].bChannel */ FREQ2FBIN(5190, 0),
			/* Data[8].ctlEdges[1].bChannel */ FREQ2FBIN(5230, 0),
			/* Data[8].ctlEdges[2].bChannel */ FREQ2FBIN(5270, 0),
			/* Data[8].ctlEdges[3].bChannel */ FREQ2FBIN(5510, 0),
			/* Data[8].ctlEdges[4].bChannel */ FREQ2FBIN(5550, 0),
			/* Data[8].ctlEdges[5].bChannel */ FREQ2FBIN(5670, 0),
			/* Data[8].ctlEdges[6].bChannel */ FREQ2FBIN(5755, 0),
			/* Data[8].ctlEdges[7].bChannel */ FREQ2FBIN(5795, 0)
		}
	},
	.ctlPowerData_5G = {
		{{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1)
		}}, {{
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0),
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0),
			CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1)
		}}
	}
};


static const struct ar9300_eeprom ar9300_x112 = {
	.eepromVersion = 2,
	.templateVersion = ar9300_tpl_ver_x112,
	.macAddr = {0x00, 0x03, 0x7f, 0x0, 0x0, 0x0},
	.custData = {"x112-041-f0000"},
	.baseEepHeader = {
		.regDmn = { LE16CONST(0x0000), LE16CONST(0x001f) },
		.txrxMask =  0x77, /* 4 bits tx and 4 bits rx */
		.opCapFlags = {
			.opFlags = AR5416_OPFLAGS_11G | AR5416_OPFLAGS_11A,
			.eepMisc = 0,
		},
		.rfSilent = 0,
		.blueToothOptions = 0,
		.deviceCap = 0,
		.deviceType = 5, /* takes lower byte in eeprom location */
		.pwrTableOffset = AR9300_PWR_TABLE_OFFSET,
		.params_for_tuning_caps = {0, 0},
		.featureEnable = 0x0d,
		/*
		 * bit0 - enable tx temp comp - disabled
		 * bit1 - enable tx volt comp - disabled
		 * bit2 - enable fastclock - enabled
		 * bit3 - enable doubling - enabled
		 * bit4 - enable internal regulator - disabled
		 * bit5 - enable pa predistortion - disabled
		 */
		.miscConfiguration = 0, /* bit0 - turn down drivestrength */
		.eepromWriteEnableGpio = 6,
		.wlanDisableGpio = 0,
		.wlanLedGpio = 8,
		.rxBandSelectGpio = 0xff,
		.txrxgain = 0x0,
		.swreg = 0,
	},
	.modalHeader2G = {
		/* ar9300_modal_eep_header  2g */
		.antCtrlCommon = LE32CONST(0x00000110),
		.antCtrlCommon2 = LE32CONST(0x00022222),
		.antCtrlChain = {
			LE16CONST(0x0010), LE16CONST(0x0010), LE16CONST(0x0010)
		},
		/*
		 * xatten1DB[AR9300_max_chains];  3 xatten1_db
		 * for ar9280 (0xa20c/b20c 5:0)
		 */
		.xatten1DB = {0x1b, 0x1b, 0x1b},

		/*
		 * xatten1Margin[ar9300_max_chains]; 3 xatten1_margin
		 * for ar9280 (0xa20c/b20c 16:12
		 */
		.xatten1Margin = {0x15, 0x15, 0x15},
		.tempSlope = 50,
		.voltSlope = 0,

		/*
		 * spurChans[OSPrey_eeprom_modal_sPURS]; spur
		 * channels in usual fbin coding format
		 */
		.spurChans = {FREQ2FBIN(2464, 1), 0, 0, 0, 0},

		/*
		 * noiseFloorThreshch[ar9300_max_cHAINS]; 3 Check
		 * if the register is per chain
		 */
		.noiseFloorThreshCh = {-1, 0, 0},
		.reserved = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.quick_drop = 0,
		.xpaBiasLvl = 0,
		.txFrameToDataStart = 0x0e,
		.txFrameToPaOn = 0x0e,
		.txClip = 3, /* 4 bits tx_clip, 4 bits dac_scale_cck */
		.antennaGain = 0,
		.switchSettling = 0x2c,
		.adcDesiredSize = -30,
		.txEndToXpaOff = 0,
		.txEndToRxOn = 0x2,
		.txFrameToXpaOn = 0xe,
		.thresh62 = 28,
		.xlna_bias_strength = 0,
		.futureModal = {
			0, 0, 0, 0, 0, 0, 0,
		},
	},
	.base_ext1 = {
		.ant_div_control = 0,
		.future = {0, 0, 0},
		.tempslopextension = {0, 0, 0, 0, 0, 0, 0, 0}
	},
	.calFreqPier2G = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2472, 1),
	},
	/* ar9300_cal_data_per_freq_op_loop 2g */
	.calPierData2G = {
		{ {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0} },
		{ {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0} },
		{ {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0} },
	},
	.calTarget_freqbin_Cck = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2472, 1),
	},
	.calTarget_freqbin_2G = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2472, 1)
	},
	.calTarget_freqbin_2GHT20 = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2472, 1)
	},
	.calTarget_freqbin_2GHT40 = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2472, 1)
	},
	.calTargetPowerCck = {
		/* 1L-5L,5S,11L,11s */
		{ { PWR2X(19), PWR2X(19), PWR2X(19), PWR2X(19) } },
		{ { PWR2X(19), PWR2X(19), PWR2X(19), PWR2X(19) } }
	},
	.calTargetPower2G = {
		/* 6-24,36,48,54 */
		{ { PWR2X(19), PWR2X(19), PWR2X(18), PWR2X(17) } },
		{ { PWR2X(19), PWR2X(19), PWR2X(18), PWR2X(17) } },
		{ { PWR2X(19), PWR2X(19), PWR2X(17), PWR2X(16) } },
	},
	.calTargetPower2GHT20 = {
		{{
			PWR2X(18), PWR2X(18),
			PWR2X(18), PWR2X(18), PWR2X(18), PWR2X(17),
			PWR2X(17), PWR2X(16), PWR2X(15), PWR2X(14),
			PWR2X(14), PWR2X(14), PWR2X(14), PWR2X(13)
		}}, {{
			PWR2X(18), PWR2X(18),
			PWR2X(18), PWR2X(18), PWR2X(18), PWR2X(17),
			PWR2X(18), PWR2X(17), PWR2X(16), PWR2X(15),
			PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(13)
		}}, {{
			PWR2X(18), PWR2X(18),
			PWR2X(18), PWR2X(18), PWR2X(18), PWR2X(17),
			PWR2X(17), PWR2X(16), PWR2X(15), PWR2X(14),
			PWR2X(14), PWR2X(14), PWR2X(14), PWR2X(13)
		}}
	},
	.calTargetPower2GHT40 = {
		{{
			PWR2X(18), PWR2X(18),
			PWR2X(18), PWR2X(18), PWR2X(17), PWR2X(16),
			PWR2X(16), PWR2X(15), PWR2X(14), PWR2X(13),
			PWR2X(13), PWR2X(13), PWR2X(13), PWR2X(12)
		}}, {{
			PWR2X(18), PWR2X(18),
			PWR2X(18), PWR2X(18), PWR2X(17), PWR2X(16),
			PWR2X(17), PWR2X(16), PWR2X(15), PWR2X(14),
			PWR2X(14), PWR2X(14), PWR2X(14), PWR2X(12)
		}}, {{
			PWR2X(18), PWR2X(18),
			PWR2X(18), PWR2X(18), PWR2X(17), PWR2X(16),
			PWR2X(16), PWR2X(15), PWR2X(14), PWR2X(13),
			PWR2X(13), PWR2X(13), PWR2X(13), PWR2X(12)
		}}
	},
	.ctlIndex_2G =  {
		0x11, 0x12, 0x15, 0x17, 0x41, 0x42,
		0x45, 0x47, 0x31, 0x32, 0x35, 0x37,
	},
	.ctl_freqbin_2G = {
		{
			FREQ2FBIN(2412, 1),
			FREQ2FBIN(2417, 1),
			FREQ2FBIN(2457, 1),
			FREQ2FBIN(2462, 1)
		},
		{
			FREQ2FBIN(2412, 1),
			FREQ2FBIN(2417, 1),
			FREQ2FBIN(2462, 1),
			0xFF,
		},

		{
			FREQ2FBIN(2412, 1),
			FREQ2FBIN(2417, 1),
			FREQ2FBIN(2462, 1),
			0xFF,
		},
		{
			FREQ2FBIN(2422, 1),
			FREQ2FBIN(2427, 1),
			FREQ2FBIN(2447, 1),
			FREQ2FBIN(2452, 1)
		},

		{
			/* Data[4].ctledges[0].bchannel */ FREQ2FBIN(2412, 1),
			/* Data[4].ctledges[1].bchannel */ FREQ2FBIN(2417, 1),
			/* Data[4].ctledges[2].bchannel */ FREQ2FBIN(2472, 1),
			/* Data[4].ctledges[3].bchannel */ FREQ2FBIN(2484, 1),
		},

		{
			/* Data[5].ctledges[0].bchannel */ FREQ2FBIN(2412, 1),
			/* Data[5].ctledges[1].bchannel */ FREQ2FBIN(2417, 1),
			/* Data[5].ctledges[2].bchannel */ FREQ2FBIN(2472, 1),
			0,
		},

		{
			/* Data[6].ctledges[0].bchannel */ FREQ2FBIN(2412, 1),
			/* Data[6].ctledges[1].bchannel */ FREQ2FBIN(2417, 1),
			FREQ2FBIN(2472, 1),
			0,
		},

		{
			/* Data[7].ctledges[0].bchannel */ FREQ2FBIN(2422, 1),
			/* Data[7].ctledges[1].bchannel */ FREQ2FBIN(2427, 1),
			/* Data[7].ctledges[2].bchannel */ FREQ2FBIN(2447, 1),
			/* Data[7].ctledges[3].bchannel */ FREQ2FBIN(2462, 1),
		},

		{
			/* Data[8].ctledges[0].bchannel */ FREQ2FBIN(2412, 1),
			/* Data[8].ctledges[1].bchannel */ FREQ2FBIN(2417, 1),
			/* Data[8].ctledges[2].bchannel */ FREQ2FBIN(2472, 1),
		},

		{
			/* Data[9].ctledges[0].bchannel */ FREQ2FBIN(2412, 1),
			/* Data[9].ctledges[1].bchannel */ FREQ2FBIN(2417, 1),
			/* Data[9].ctledges[2].bchannel */ FREQ2FBIN(2472, 1),
			0
		},

		{
			/* Data[10].ctledges[0].bchannel */ FREQ2FBIN(2412, 1),
			/* Data[10].ctledges[1].bchannel */ FREQ2FBIN(2417, 1),
			/* Data[10].ctledges[2].bchannel */ FREQ2FBIN(2472, 1),
			0
		},

		{
			/* Data[11].ctledges[0].bchannel */ FREQ2FBIN(2422, 1),
			/* Data[11].ctledges[1].bchannel */ FREQ2FBIN(2427, 1),
			/* Data[11].ctledges[2].bchannel */ FREQ2FBIN(2447, 1),
			/* Data[11].ctledges[3].bchannel */ FREQ2FBIN(2462, 1),
		}
	},
	.ctlPowerData_2G = {
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 1) } },

		{ { CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },

		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },

		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1) } }
	},
	.modalHeader5G = {
		.antCtrlCommon = LE32CONST(0x00000110),
		.antCtrlCommon2 = LE32CONST(0x00022222),
		/* xatten1DB 3 xatten1_db for ar9280 (0xa20c/b20c 5:0) */
		.xatten1DB = {0x13, 0x19, 0x17},

		/*
		 * xatten1Margin[ar9300_max_chains]; 3 xatten1_margin
		 * for merlin (0xa20c/b20c 16:12
		 */
		.xatten1Margin = {0x19, 0x19, 0x19},
		.tempSlope = 70,
		.voltSlope = 15,
		/* spurChans spur channels in usual fbin coding format */
		.spurChans = {0, 0, 0, 0, 0},
		/* noiseFloorThreshch check if the register is per chain */
		.noiseFloorThreshCh = {-1, 0, 0},
		.reserved = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.quick_drop = 0,
		.xpaBiasLvl = 0,
		.txFrameToDataStart = 0x0e,
		.txFrameToPaOn = 0x0e,
		.txClip = 3, /* 4 bits tx_clip, 4 bits dac_scale_cck */
		.antennaGain = 0,
		.switchSettling = 0x2d,
		.adcDesiredSize = -30,
		.txEndToXpaOff = 0,
		.txEndToRxOn = 0x2,
		.txFrameToXpaOn = 0xe,
		.thresh62 = 28,
		.xlna_bias_strength = 0,
		.futureModal = {
			0, 0, 0, 0, 0, 0, 0,
		},
	},
	.base_ext2 = {
		.tempSlopeLow = 72,
		.tempSlopeHigh = 105,
		.xatten1DBLow = {0x10, 0x14, 0x10},
		.xatten1MarginLow = {0x19, 0x19 , 0x19},
		.xatten1DBHigh = {0x1d, 0x20, 0x24},
		.xatten1MarginHigh = {0x10, 0x10, 0x10}
	},
	.calFreqPier5G = {
		FREQ2FBIN(5180, 0),
		FREQ2FBIN(5220, 0),
		FREQ2FBIN(5320, 0),
		FREQ2FBIN(5400, 0),
		FREQ2FBIN(5500, 0),
		FREQ2FBIN(5600, 0),
		FREQ2FBIN(5700, 0),
		FREQ2FBIN(5785, 0)
	},
	.calPierData5G = {
		{
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
		},
		{
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
		},
		{
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0},
		},

	},
	.calTarget_freqbin_5G = {
		FREQ2FBIN(5180, 0),
		FREQ2FBIN(5220, 0),
		FREQ2FBIN(5320, 0),
		FREQ2FBIN(5400, 0),
		FREQ2FBIN(5500, 0),
		FREQ2FBIN(5600, 0),
		FREQ2FBIN(5725, 0),
		FREQ2FBIN(5825, 0)
	},
	.calTarget_freqbin_5GHT20 = {
		FREQ2FBIN(5180, 0),
		FREQ2FBIN(5220, 0),
		FREQ2FBIN(5320, 0),
		FREQ2FBIN(5400, 0),
		FREQ2FBIN(5500, 0),
		FREQ2FBIN(5600, 0),
		FREQ2FBIN(5725, 0),
		FREQ2FBIN(5825, 0)
	},
	.calTarget_freqbin_5GHT40 = {
		FREQ2FBIN(5180, 0),
		FREQ2FBIN(5220, 0),
		FREQ2FBIN(5320, 0),
		FREQ2FBIN(5400, 0),
		FREQ2FBIN(5500, 0),
		FREQ2FBIN(5600, 0),
		FREQ2FBIN(5725, 0),
		FREQ2FBIN(5825, 0)
	},
	.calTargetPower5G = {
		/* 6-24,36,48,54 */
		{ { PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(13) } },
		{ { PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(13) } },
		{ { PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(13) } },
		{ { PWR2X(16), PWR2X(16), PWR2X(13), PWR2X(12) } },
		{ { PWR2X(16), PWR2X(16), PWR2X(13), PWR2X(12) } },
		{ { PWR2X(16), PWR2X(16), PWR2X(12), PWR2X(11) } },
		{ { PWR2X(15), PWR2X(15), PWR2X(12), PWR2X(11) } },
		{ { PWR2X(15), PWR2X(15), PWR2X(12), PWR2X(11) } }
	},
	.calTargetPower5GHT20 = {
		/*
		 * 0_8_16,1-3_9-11_17-19,
		 * 4,5,6,7,
		 * 12,13,14,15,
		 * 20,21,22,23
		 */
		{{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(13),
			PWR2X(16), PWR2X(14), PWR2X(13), PWR2X(12),
			PWR2X(12), PWR2X(12), PWR2X(11), PWR2X(11)
		}}, {{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(13),
			PWR2X(16), PWR2X(14), PWR2X(13), PWR2X(12),
			PWR2X(12), PWR2X(12), PWR2X(11), PWR2X(11)
		}}, {{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(13),
			PWR2X(16), PWR2X(14), PWR2X(13), PWR2X(12),
			PWR2X(12), PWR2X(12), PWR2X(11), PWR2X(11)
		}}, {{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(13),
			PWR2X(16), PWR2X(13), PWR2X(12), PWR2X(11),
			PWR2X(11), PWR2X(11), PWR2X(10), PWR2X(10)
		}}, {{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(13),
			PWR2X(16), PWR2X(13), PWR2X(12), PWR2X(11),
			PWR2X(10), PWR2X(9), PWR2X(8), PWR2X(8)
		}}, {{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(16), PWR2X(14), PWR2X(13),
			PWR2X(16), PWR2X(12), PWR2X(10), PWR2X(8),
			PWR2X(9), PWR2X(8), PWR2X(7), PWR2X(7)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(13),
			PWR2X(15), PWR2X(12), PWR2X(10), PWR2X(8),
			PWR2X(9), PWR2X(8), PWR2X(7), PWR2X(7)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(13),
			PWR2X(15), PWR2X(12), PWR2X(10), PWR2X(8),
			PWR2X(9), PWR2X(8), PWR2X(7), PWR2X(7)
		}}
	},
	.calTargetPower5GHT40 =  {
		/*
		 * 0_8_16,1-3_9-11_17-19,
		 * 4,5,6,7,
		 * 12,13,14,15,
		 * 20,21,22,23
		 */
		{{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(15), PWR2X(14), PWR2X(13),
			PWR2X(15), PWR2X(14), PWR2X(13), PWR2X(12),
			PWR2X(12), PWR2X(12), PWR2X(11), PWR2X(11)
		}}, {{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(15), PWR2X(14), PWR2X(13),
			PWR2X(15), PWR2X(14), PWR2X(13), PWR2X(12),
			PWR2X(12), PWR2X(12), PWR2X(11), PWR2X(11)
		}}, {{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(15), PWR2X(14), PWR2X(13),
			PWR2X(15), PWR2X(14), PWR2X(13), PWR2X(12),
			PWR2X(12), PWR2X(12), PWR2X(11), PWR2X(11)
		}}, {{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(15), PWR2X(14), PWR2X(13),
			PWR2X(15), PWR2X(13), PWR2X(12), PWR2X(11),
			PWR2X(11), PWR2X(11), PWR2X(10), PWR2X(10)
		}}, {{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(15), PWR2X(14), PWR2X(13),
			PWR2X(15), PWR2X(13), PWR2X(12), PWR2X(11),
			PWR2X(10), PWR2X(9), PWR2X(8), PWR2X(8)
		}}, {{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(15), PWR2X(14), PWR2X(13),
			PWR2X(15), PWR2X(11), PWR2X(10), PWR2X(8),
			PWR2X(9), PWR2X(8), PWR2X(7), PWR2X(7)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(13),
			PWR2X(15), PWR2X(11), PWR2X(10), PWR2X(8),
			PWR2X(9), PWR2X(8), PWR2X(7), PWR2X(7)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(13),
			PWR2X(15), PWR2X(11), PWR2X(10), PWR2X(8),
			PWR2X(9), PWR2X(8), PWR2X(7), PWR2X(7)
		}}
	},
	.ctlIndex_5G =  {
		0x10, 0x16, 0x18, 0x40, 0x46,
		0x48, 0x30, 0x36, 0x38
	},
	.ctl_freqbin_5G =  {
		{
			/* Data[0].ctledges[0].bchannel */ FREQ2FBIN(5180, 0),
			/* Data[0].ctledges[1].bchannel */ FREQ2FBIN(5260, 0),
			/* Data[0].ctledges[2].bchannel */ FREQ2FBIN(5280, 0),
			/* Data[0].ctledges[3].bchannel */ FREQ2FBIN(5500, 0),
			/* Data[0].ctledges[4].bchannel */ FREQ2FBIN(5600, 0),
			/* Data[0].ctledges[5].bchannel */ FREQ2FBIN(5700, 0),
			/* Data[0].ctledges[6].bchannel */ FREQ2FBIN(5745, 0),
			/* Data[0].ctledges[7].bchannel */ FREQ2FBIN(5825, 0)
		},
		{
			/* Data[1].ctledges[0].bchannel */ FREQ2FBIN(5180, 0),
			/* Data[1].ctledges[1].bchannel */ FREQ2FBIN(5260, 0),
			/* Data[1].ctledges[2].bchannel */ FREQ2FBIN(5280, 0),
			/* Data[1].ctledges[3].bchannel */ FREQ2FBIN(5500, 0),
			/* Data[1].ctledges[4].bchannel */ FREQ2FBIN(5520, 0),
			/* Data[1].ctledges[5].bchannel */ FREQ2FBIN(5700, 0),
			/* Data[1].ctledges[6].bchannel */ FREQ2FBIN(5745, 0),
			/* Data[1].ctledges[7].bchannel */ FREQ2FBIN(5825, 0)
		},

		{
			/* Data[2].ctledges[0].bchannel */ FREQ2FBIN(5190, 0),
			/* Data[2].ctledges[1].bchannel */ FREQ2FBIN(5230, 0),
			/* Data[2].ctledges[2].bchannel */ FREQ2FBIN(5270, 0),
			/* Data[2].ctledges[3].bchannel */ FREQ2FBIN(5310, 0),
			/* Data[2].ctledges[4].bchannel */ FREQ2FBIN(5510, 0),
			/* Data[2].ctledges[5].bchannel */ FREQ2FBIN(5550, 0),
			/* Data[2].ctledges[6].bchannel */ FREQ2FBIN(5670, 0),
			/* Data[2].ctledges[7].bchannel */ FREQ2FBIN(5755, 0)
		},

		{
			/* Data[3].ctledges[0].bchannel */ FREQ2FBIN(5180, 0),
			/* Data[3].ctledges[1].bchannel */ FREQ2FBIN(5200, 0),
			/* Data[3].ctledges[2].bchannel */ FREQ2FBIN(5260, 0),
			/* Data[3].ctledges[3].bchannel */ FREQ2FBIN(5320, 0),
			/* Data[3].ctledges[4].bchannel */ FREQ2FBIN(5500, 0),
			/* Data[3].ctledges[5].bchannel */ FREQ2FBIN(5700, 0),
			/* Data[3].ctledges[6].bchannel */ 0xFF,
			/* Data[3].ctledges[7].bchannel */ 0xFF,
		},

		{
			/* Data[4].ctledges[0].bchannel */ FREQ2FBIN(5180, 0),
			/* Data[4].ctledges[1].bchannel */ FREQ2FBIN(5260, 0),
			/* Data[4].ctledges[2].bchannel */ FREQ2FBIN(5500, 0),
			/* Data[4].ctledges[3].bchannel */ FREQ2FBIN(5700, 0),
			/* Data[4].ctledges[4].bchannel */ 0xFF,
			/* Data[4].ctledges[5].bchannel */ 0xFF,
			/* Data[4].ctledges[6].bchannel */ 0xFF,
			/* Data[4].ctledges[7].bchannel */ 0xFF,
		},

		{
			/* Data[5].ctledges[0].bchannel */ FREQ2FBIN(5190, 0),
			/* Data[5].ctledges[1].bchannel */ FREQ2FBIN(5270, 0),
			/* Data[5].ctledges[2].bchannel */ FREQ2FBIN(5310, 0),
			/* Data[5].ctledges[3].bchannel */ FREQ2FBIN(5510, 0),
			/* Data[5].ctledges[4].bchannel */ FREQ2FBIN(5590, 0),
			/* Data[5].ctledges[5].bchannel */ FREQ2FBIN(5670, 0),
			/* Data[5].ctledges[6].bchannel */ 0xFF,
			/* Data[5].ctledges[7].bchannel */ 0xFF
		},

		{
			/* Data[6].ctledges[0].bchannel */ FREQ2FBIN(5180, 0),
			/* Data[6].ctledges[1].bchannel */ FREQ2FBIN(5200, 0),
			/* Data[6].ctledges[2].bchannel */ FREQ2FBIN(5220, 0),
			/* Data[6].ctledges[3].bchannel */ FREQ2FBIN(5260, 0),
			/* Data[6].ctledges[4].bchannel */ FREQ2FBIN(5500, 0),
			/* Data[6].ctledges[5].bchannel */ FREQ2FBIN(5600, 0),
			/* Data[6].ctledges[6].bchannel */ FREQ2FBIN(5700, 0),
			/* Data[6].ctledges[7].bchannel */ FREQ2FBIN(5745, 0)
		},

		{
			/* Data[7].ctledges[0].bchannel */ FREQ2FBIN(5180, 0),
			/* Data[7].ctledges[1].bchannel */ FREQ2FBIN(5260, 0),
			/* Data[7].ctledges[2].bchannel */ FREQ2FBIN(5320, 0),
			/* Data[7].ctledges[3].bchannel */ FREQ2FBIN(5500, 0),
			/* Data[7].ctledges[4].bchannel */ FREQ2FBIN(5560, 0),
			/* Data[7].ctledges[5].bchannel */ FREQ2FBIN(5700, 0),
			/* Data[7].ctledges[6].bchannel */ FREQ2FBIN(5745, 0),
			/* Data[7].ctledges[7].bchannel */ FREQ2FBIN(5825, 0)
		},

		{
			/* Data[8].ctledges[0].bchannel */ FREQ2FBIN(5190, 0),
			/* Data[8].ctledges[1].bchannel */ FREQ2FBIN(5230, 0),
			/* Data[8].ctledges[2].bchannel */ FREQ2FBIN(5270, 0),
			/* Data[8].ctledges[3].bchannel */ FREQ2FBIN(5510, 0),
			/* Data[8].ctledges[4].bchannel */ FREQ2FBIN(5550, 0),
			/* Data[8].ctledges[5].bchannel */ FREQ2FBIN(5670, 0),
			/* Data[8].ctledges[6].bchannel */ FREQ2FBIN(5755, 0),
			/* Data[8].ctledges[7].bchannel */ FREQ2FBIN(5795, 0)
		}
	},
	.ctlPowerData_5G = {
		{{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1)
		}}, {{
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0),
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0),
			CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1)
		}}
	}
};

static const struct ar9300_eeprom ar9300_h116 = {
	.eepromVersion = 2,
	.templateVersion = ar9300_tpl_ver_h116,
	.macAddr = {0x00, 0x03, 0x7f, 0x0, 0x0, 0x0},
	.custData = {"h116-041-f0000"},
	.baseEepHeader = {
		.regDmn = { LE16CONST(0x0000), LE16CONST(0x001f) },
		.txrxMask =  0x33, /* 4 bits tx and 4 bits rx */
		.opCapFlags = {
			.opFlags = AR5416_OPFLAGS_11G | AR5416_OPFLAGS_11A,
			.eepMisc = 0,
		},
		.rfSilent = 0,
		.blueToothOptions = 0,
		.deviceCap = 0,
		.deviceType = 5, /* takes lower byte in eeprom location */
		.pwrTableOffset = AR9300_PWR_TABLE_OFFSET,
		.params_for_tuning_caps = {0, 0},
		.featureEnable = 0x0d,
		/*
		 * bit0 - enable tx temp comp - disabled
		 * bit1 - enable tx volt comp - disabled
		 * bit2 - enable fastClock - enabled
		 * bit3 - enable doubling - enabled
		 * bit4 - enable internal regulator - disabled
		 * bit5 - enable pa predistortion - disabled
		 */
		.miscConfiguration = 0, /* bit0 - turn down drivestrength */
		.eepromWriteEnableGpio = 6,
		.wlanDisableGpio = 0,
		.wlanLedGpio = 8,
		.rxBandSelectGpio = 0xff,
		.txrxgain = 0x10,
		.swreg = 0,
	},
	.modalHeader2G = {
	/* ar9300_modal_eep_header  2g */
		.antCtrlCommon = LE32CONST(0x00000110),
		.antCtrlCommon2 = LE32CONST(0x00044444),
		.antCtrlChain = {
			LE16CONST(0x0010), LE16CONST(0x0010), LE16CONST(0x0010)
		},
		/*
		 * xatten1DB[AR9300_MAX_CHAINS];  3 xatten1_db
		 * for ar9280 (0xa20c/b20c 5:0)
		 */
		.xatten1DB = {0x1f, 0x1f, 0x1f},

		/*
		 * xatten1Margin[AR9300_MAX_CHAINS]; 3 xatten1_margin
		 * for ar9280 (0xa20c/b20c 16:12
		 */
		.xatten1Margin = {0x12, 0x12, 0x12},
		.tempSlope = 25,
		.voltSlope = 0,

		/*
		 * spurChans[OSPREY_EEPROM_MODAL_SPURS]; spur
		 * channels in usual fbin coding format
		 */
		.spurChans = {FREQ2FBIN(2464, 1), 0, 0, 0, 0},

		/*
		 * noiseFloorThreshCh[AR9300_MAX_CHAINS]; 3 Check
		 * if the register is per chain
		 */
		.noiseFloorThreshCh = {-1, 0, 0},
		.reserved = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.quick_drop = 0,
		.xpaBiasLvl = 0,
		.txFrameToDataStart = 0x0e,
		.txFrameToPaOn = 0x0e,
		.txClip = 3, /* 4 bits tx_clip, 4 bits dac_scale_cck */
		.antennaGain = 0,
		.switchSettling = 0x2c,
		.adcDesiredSize = -30,
		.txEndToXpaOff = 0,
		.txEndToRxOn = 0x2,
		.txFrameToXpaOn = 0xe,
		.thresh62 = 28,
		.xlna_bias_strength = 0,
		.futureModal = {
			0, 0, 0, 0, 0, 0, 0,
		},
	},
	.base_ext1 = {
		.ant_div_control = 0,
		.future = {0, 0, 0},
		.tempslopextension = {0, 0, 0, 0, 0, 0, 0, 0}
	},
	.calFreqPier2G = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2462, 1),
	},
	/* ar9300_cal_data_per_freq_op_loop 2g */
	.calPierData2G = {
		{ {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0} },
		{ {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0} },
		{ {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0} },
	},
	.calTarget_freqbin_Cck = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2472, 1),
	},
	.calTarget_freqbin_2G = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2472, 1)
	},
	.calTarget_freqbin_2GHT20 = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2472, 1)
	},
	.calTarget_freqbin_2GHT40 = {
		FREQ2FBIN(2412, 1),
		FREQ2FBIN(2437, 1),
		FREQ2FBIN(2472, 1)
	},
	.calTargetPowerCck = {
		/* 1L-5L,5S,11L,11S */
		{ { PWR2X(17), PWR2X(17), PWR2X(17), PWR2X(17) } },
		{ { PWR2X(17), PWR2X(17), PWR2X(17), PWR2X(17) } }
	},
	.calTargetPower2G = {
		/* 6-24,36,48,54 */
		{ { PWR2X(17), PWR2X(17), PWR2X(16), PWR2X(16) } },
		{ { PWR2X(17), PWR2X(17), PWR2X(16), PWR2X(16) } },
		{ { PWR2X(17), PWR2X(17), PWR2X(16), PWR2X(16) } }
	},
	.calTargetPower2GHT20 = {
		{{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(16), PWR2X(16), PWR2X(15),
			PWR2X(16), PWR2X(16), PWR2X(15), PWR2X(14)
		}}, {{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(16), PWR2X(16), PWR2X(15),
			PWR2X(16), PWR2X(16), PWR2X(15), PWR2X(14)
		}}, {{
			PWR2X(16), PWR2X(16),
			PWR2X(16), PWR2X(16), PWR2X(16), PWR2X(15),
			PWR2X(16), PWR2X(16), PWR2X(15), PWR2X(14)
		}}
	},
	.calTargetPower2GHT40 = {
		{{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(15), PWR2X(15), PWR2X(14),
			PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(13)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(15), PWR2X(15), PWR2X(14),
			PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(13)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(15), PWR2X(15), PWR2X(14),
			PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(13)
		}}
	},
	.ctlIndex_2G =  {
		0x11, 0x12, 0x15, 0x17, 0x41, 0x42,
		0x45, 0x47, 0x31, 0x32, 0x35, 0x37,
	},
	.ctl_freqbin_2G = {
		{
			FREQ2FBIN(2412, 1),
			FREQ2FBIN(2417, 1),
			FREQ2FBIN(2457, 1),
			FREQ2FBIN(2462, 1)
		},
		{
			FREQ2FBIN(2412, 1),
			FREQ2FBIN(2417, 1),
			FREQ2FBIN(2462, 1),
			0xFF,
		},

		{
			FREQ2FBIN(2412, 1),
			FREQ2FBIN(2417, 1),
			FREQ2FBIN(2462, 1),
			0xFF,
		},
		{
			FREQ2FBIN(2422, 1),
			FREQ2FBIN(2427, 1),
			FREQ2FBIN(2447, 1),
			FREQ2FBIN(2452, 1)
		},

		{
			/* Data[4].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[4].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			/* Data[4].ctlEdges[2].bChannel */ FREQ2FBIN(2472, 1),
			/* Data[4].ctlEdges[3].bChannel */ FREQ2FBIN(2484, 1),
		},

		{
			/* Data[5].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[5].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			/* Data[5].ctlEdges[2].bChannel */ FREQ2FBIN(2472, 1),
			0,
		},

		{
			/* Data[6].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[6].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			FREQ2FBIN(2472, 1),
			0,
		},

		{
			/* Data[7].ctlEdges[0].bChannel */ FREQ2FBIN(2422, 1),
			/* Data[7].ctlEdges[1].bChannel */ FREQ2FBIN(2427, 1),
			/* Data[7].ctlEdges[2].bChannel */ FREQ2FBIN(2447, 1),
			/* Data[7].ctlEdges[3].bChannel */ FREQ2FBIN(2462, 1),
		},

		{
			/* Data[8].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[8].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			/* Data[8].ctlEdges[2].bChannel */ FREQ2FBIN(2472, 1),
		},

		{
			/* Data[9].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[9].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			/* Data[9].ctlEdges[2].bChannel */ FREQ2FBIN(2472, 1),
			0
		},

		{
			/* Data[10].ctlEdges[0].bChannel */ FREQ2FBIN(2412, 1),
			/* Data[10].ctlEdges[1].bChannel */ FREQ2FBIN(2417, 1),
			/* Data[10].ctlEdges[2].bChannel */ FREQ2FBIN(2472, 1),
			0
		},

		{
			/* Data[11].ctlEdges[0].bChannel */ FREQ2FBIN(2422, 1),
			/* Data[11].ctlEdges[1].bChannel */ FREQ2FBIN(2427, 1),
			/* Data[11].ctlEdges[2].bChannel */ FREQ2FBIN(2447, 1),
			/* Data[11].ctlEdges[3].bChannel */ FREQ2FBIN(2462, 1),
		}
	},
	.ctlPowerData_2G = {
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 1) } },

		{ { CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },

		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },

		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1) } },
		{ { CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1) } }
	},
	.modalHeader5G = {
		.antCtrlCommon = LE32CONST(0x00000220),
		.antCtrlCommon2 = LE32CONST(0x00044444),
		.antCtrlChain = {
			LE16CONST(0x0150), LE16CONST(0x0150), LE16CONST(0x0150)
		},
		/* xatten1DB 3 xatten1_db for AR9280 (0xa20c/b20c 5:0) */
		.xatten1DB = {0x19, 0x19, 0x19},

		/*
		 * xatten1Margin[AR9300_MAX_CHAINS]; 3 xatten1_margin
		 * for merlin (0xa20c/b20c 16:12
		 */
		.xatten1Margin = {0x14, 0x14, 0x14},
		.tempSlope = 70,
		.voltSlope = 0,
		/* spurChans spur channels in usual fbin coding format */
		.spurChans = {0, 0, 0, 0, 0},
		/* noiseFloorThreshCh Check if the register is per chain */
		.noiseFloorThreshCh = {-1, 0, 0},
		.reserved = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.quick_drop = 0,
		.xpaBiasLvl = 0,
		.txFrameToDataStart = 0x0e,
		.txFrameToPaOn = 0x0e,
		.txClip = 3, /* 4 bits tx_clip, 4 bits dac_scale_cck */
		.antennaGain = 0,
		.switchSettling = 0x2d,
		.adcDesiredSize = -30,
		.txEndToXpaOff = 0,
		.txEndToRxOn = 0x2,
		.txFrameToXpaOn = 0xe,
		.thresh62 = 28,
		.xlna_bias_strength = 0,
		.futureModal = {
			0, 0, 0, 0, 0, 0, 0,
		},
	},
	.base_ext2 = {
		.tempSlopeLow = 35,
		.tempSlopeHigh = 50,
		.xatten1DBLow = {0, 0, 0},
		.xatten1MarginLow = {0, 0, 0},
		.xatten1DBHigh = {0, 0, 0},
		.xatten1MarginHigh = {0, 0, 0}
	},
	.calFreqPier5G = {
		FREQ2FBIN(5160, 0),
		FREQ2FBIN(5220, 0),
		FREQ2FBIN(5320, 0),
		FREQ2FBIN(5400, 0),
		FREQ2FBIN(5500, 0),
		FREQ2FBIN(5600, 0),
		FREQ2FBIN(5700, 0),
		FREQ2FBIN(5785, 0)
	},
	.calPierData5G = {
			{
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
			},
			{
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
			},
			{
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0},
			},

	},
	.calTarget_freqbin_5G = {
		FREQ2FBIN(5180, 0),
		FREQ2FBIN(5240, 0),
		FREQ2FBIN(5320, 0),
		FREQ2FBIN(5400, 0),
		FREQ2FBIN(5500, 0),
		FREQ2FBIN(5600, 0),
		FREQ2FBIN(5700, 0),
		FREQ2FBIN(5825, 0)
	},
	.calTarget_freqbin_5GHT20 = {
		FREQ2FBIN(5180, 0),
		FREQ2FBIN(5240, 0),
		FREQ2FBIN(5320, 0),
		FREQ2FBIN(5400, 0),
		FREQ2FBIN(5500, 0),
		FREQ2FBIN(5700, 0),
		FREQ2FBIN(5745, 0),
		FREQ2FBIN(5825, 0)
	},
	.calTarget_freqbin_5GHT40 = {
		FREQ2FBIN(5180, 0),
		FREQ2FBIN(5240, 0),
		FREQ2FBIN(5320, 0),
		FREQ2FBIN(5400, 0),
		FREQ2FBIN(5500, 0),
		FREQ2FBIN(5700, 0),
		FREQ2FBIN(5745, 0),
		FREQ2FBIN(5825, 0)
	},
	.calTargetPower5G = {
		/* 6-24,36,48,54 */
		{ { PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(12) } },
		{ { PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(12) } },
		{ { PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(12) } },
		{ { PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(12) } },
		{ { PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(12) } },
		{ { PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(12) } },
		{ { PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(12) } },
		{ { PWR2X(15), PWR2X(15), PWR2X(14), PWR2X(12) } }
	},
	.calTargetPower5GHT20 = {
		/*
		 * 0_8_16,1-3_9-11_17-19,
		 * 4,5,6,7,
		 * 12,13,14,15,
		 * 20,21,22,23
		 */
		{{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(14), PWR2X(12), PWR2X(10),
			PWR2X(15), PWR2X(14), PWR2X(12), PWR2X(10)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(14), PWR2X(12), PWR2X(10),
			PWR2X(15), PWR2X(14), PWR2X(12), PWR2X(10)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(13), PWR2X(11), PWR2X(9),
			PWR2X(15), PWR2X(13), PWR2X(11), PWR2X(9)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(13), PWR2X(11), PWR2X(9),
			PWR2X(15), PWR2X(13), PWR2X(11), PWR2X(9)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(12), PWR2X(10), PWR2X(8),
			PWR2X(15), PWR2X(12), PWR2X(10), PWR2X(8)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(12), PWR2X(10), PWR2X(8),
			PWR2X(15), PWR2X(12), PWR2X(10), PWR2X(8)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(11), PWR2X(9), PWR2X(7),
			PWR2X(15), PWR2X(11), PWR2X(9), PWR2X(7)
		}}, {{
			PWR2X(15), PWR2X(15),
			PWR2X(15), PWR2X(11), PWR2X(9), PWR2X(7),
			PWR2X(15), PWR2X(11), PWR2X(9), PWR2X(7)
		}}
	},
	.calTargetPower5GHT40 =  {
		/*
		 * 0_8_16,1-3_9-11_17-19,
		 * 4,5,6,7,12,13,14,15,20,21,22,23
		 */
		{{
			PWR2X(14), PWR2X(14),
			PWR2X(14), PWR2X(13), PWR2X(11), PWR2X(9),
			PWR2X(14), PWR2X(13), PWR2X(11), PWR2X(9)
		}}, {{
			PWR2X(14), PWR2X(14),
			PWR2X(14), PWR2X(13), PWR2X(11), PWR2X(9),
			PWR2X(14), PWR2X(13), PWR2X(11), PWR2X(9)
		}}, {{
			PWR2X(14), PWR2X(14),
			PWR2X(14), PWR2X(12), PWR2X(10), PWR2X(8),
			PWR2X(14), PWR2X(12), PWR2X(10), PWR2X(8)
		}}, {{
			PWR2X(14), PWR2X(14),
			PWR2X(14), PWR2X(12), PWR2X(10), PWR2X(8),
			PWR2X(14), PWR2X(12), PWR2X(10), PWR2X(8)
		}}, {{
			PWR2X(14), PWR2X(14),
			PWR2X(14), PWR2X(11), PWR2X(9), PWR2X(7),
			PWR2X(14), PWR2X(11), PWR2X(9), PWR2X(7)
		}}, {{
			PWR2X(14), PWR2X(14),
			PWR2X(14), PWR2X(11), PWR2X(9), PWR2X(7),
			PWR2X(14), PWR2X(11), PWR2X(9), PWR2X(7)
		}}, {{
			PWR2X(14), PWR2X(14),
			PWR2X(14), PWR2X(10), PWR2X(8), PWR2X(6),
			PWR2X(14), PWR2X(10), PWR2X(8), PWR2X(6)
		}}, {{
			PWR2X(14), PWR2X(14),
			PWR2X(14), PWR2X(10), PWR2X(8), PWR2X(6),
			PWR2X(14), PWR2X(10), PWR2X(8), PWR2X(6)
		}}
	},
	.ctlIndex_5G =  {
		0x10, 0x16, 0x18, 0x40, 0x46,
		0x48, 0x30, 0x36, 0x38
	},
	.ctl_freqbin_5G =  {
		{
			/* Data[0].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[0].ctlEdges[1].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[0].ctlEdges[2].bChannel */ FREQ2FBIN(5280, 0),
			/* Data[0].ctlEdges[3].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[0].ctlEdges[4].bChannel */ FREQ2FBIN(5600, 0),
			/* Data[0].ctlEdges[5].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[0].ctlEdges[6].bChannel */ FREQ2FBIN(5745, 0),
			/* Data[0].ctlEdges[7].bChannel */ FREQ2FBIN(5825, 0)
		},
		{
			/* Data[1].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[1].ctlEdges[1].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[1].ctlEdges[2].bChannel */ FREQ2FBIN(5280, 0),
			/* Data[1].ctlEdges[3].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[1].ctlEdges[4].bChannel */ FREQ2FBIN(5520, 0),
			/* Data[1].ctlEdges[5].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[1].ctlEdges[6].bChannel */ FREQ2FBIN(5745, 0),
			/* Data[1].ctlEdges[7].bChannel */ FREQ2FBIN(5825, 0)
		},

		{
			/* Data[2].ctlEdges[0].bChannel */ FREQ2FBIN(5190, 0),
			/* Data[2].ctlEdges[1].bChannel */ FREQ2FBIN(5230, 0),
			/* Data[2].ctlEdges[2].bChannel */ FREQ2FBIN(5270, 0),
			/* Data[2].ctlEdges[3].bChannel */ FREQ2FBIN(5310, 0),
			/* Data[2].ctlEdges[4].bChannel */ FREQ2FBIN(5510, 0),
			/* Data[2].ctlEdges[5].bChannel */ FREQ2FBIN(5550, 0),
			/* Data[2].ctlEdges[6].bChannel */ FREQ2FBIN(5670, 0),
			/* Data[2].ctlEdges[7].bChannel */ FREQ2FBIN(5755, 0)
		},

		{
			/* Data[3].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[3].ctlEdges[1].bChannel */ FREQ2FBIN(5200, 0),
			/* Data[3].ctlEdges[2].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[3].ctlEdges[3].bChannel */ FREQ2FBIN(5320, 0),
			/* Data[3].ctlEdges[4].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[3].ctlEdges[5].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[3].ctlEdges[6].bChannel */ 0xFF,
			/* Data[3].ctlEdges[7].bChannel */ 0xFF,
		},

		{
			/* Data[4].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[4].ctlEdges[1].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[4].ctlEdges[2].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[4].ctlEdges[3].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[4].ctlEdges[4].bChannel */ 0xFF,
			/* Data[4].ctlEdges[5].bChannel */ 0xFF,
			/* Data[4].ctlEdges[6].bChannel */ 0xFF,
			/* Data[4].ctlEdges[7].bChannel */ 0xFF,
		},

		{
			/* Data[5].ctlEdges[0].bChannel */ FREQ2FBIN(5190, 0),
			/* Data[5].ctlEdges[1].bChannel */ FREQ2FBIN(5270, 0),
			/* Data[5].ctlEdges[2].bChannel */ FREQ2FBIN(5310, 0),
			/* Data[5].ctlEdges[3].bChannel */ FREQ2FBIN(5510, 0),
			/* Data[5].ctlEdges[4].bChannel */ FREQ2FBIN(5590, 0),
			/* Data[5].ctlEdges[5].bChannel */ FREQ2FBIN(5670, 0),
			/* Data[5].ctlEdges[6].bChannel */ 0xFF,
			/* Data[5].ctlEdges[7].bChannel */ 0xFF
		},

		{
			/* Data[6].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[6].ctlEdges[1].bChannel */ FREQ2FBIN(5200, 0),
			/* Data[6].ctlEdges[2].bChannel */ FREQ2FBIN(5220, 0),
			/* Data[6].ctlEdges[3].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[6].ctlEdges[4].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[6].ctlEdges[5].bChannel */ FREQ2FBIN(5600, 0),
			/* Data[6].ctlEdges[6].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[6].ctlEdges[7].bChannel */ FREQ2FBIN(5745, 0)
		},

		{
			/* Data[7].ctlEdges[0].bChannel */ FREQ2FBIN(5180, 0),
			/* Data[7].ctlEdges[1].bChannel */ FREQ2FBIN(5260, 0),
			/* Data[7].ctlEdges[2].bChannel */ FREQ2FBIN(5320, 0),
			/* Data[7].ctlEdges[3].bChannel */ FREQ2FBIN(5500, 0),
			/* Data[7].ctlEdges[4].bChannel */ FREQ2FBIN(5560, 0),
			/* Data[7].ctlEdges[5].bChannel */ FREQ2FBIN(5700, 0),
			/* Data[7].ctlEdges[6].bChannel */ FREQ2FBIN(5745, 0),
			/* Data[7].ctlEdges[7].bChannel */ FREQ2FBIN(5825, 0)
		},

		{
			/* Data[8].ctlEdges[0].bChannel */ FREQ2FBIN(5190, 0),
			/* Data[8].ctlEdges[1].bChannel */ FREQ2FBIN(5230, 0),
			/* Data[8].ctlEdges[2].bChannel */ FREQ2FBIN(5270, 0),
			/* Data[8].ctlEdges[3].bChannel */ FREQ2FBIN(5510, 0),
			/* Data[8].ctlEdges[4].bChannel */ FREQ2FBIN(5550, 0),
			/* Data[8].ctlEdges[5].bChannel */ FREQ2FBIN(5670, 0),
			/* Data[8].ctlEdges[6].bChannel */ FREQ2FBIN(5755, 0),
			/* Data[8].ctlEdges[7].bChannel */ FREQ2FBIN(5795, 0)
		}
	},
	.ctlPowerData_5G = {
		{{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1)
		}}, {{
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0),
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0),
			CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0)
		}}, {{
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 1), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1)
		}}
	}
};

#endif
