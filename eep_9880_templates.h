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

#ifndef EEP_9880_TEMPLATES_H
#define EEP_9880_TEMPLATES_H

/**
 * gcc does not like to initialize from structure field, so define template
 * versions independently and initialize both template and index with enum
 * values. Use lower case to facilitate index filling with macro.
 */
enum qca9880_template_versions {
	qca9880_tpl_ver_cus223 = 3,
	qca9880_tpl_ver_xb140 = 14,
};

#define PWR2XVHTBASE(_b0, _b1, _b2)					\
		{ PWR2X(_b0), PWR2X(_b1), PWR2X(_b2) }
#define PWR2XVHTDELTA(_d0, _d1, _d2, _d3, _d4, _d5, _d6, _d7, _d8, \
		      _d9, _d10, _d11, _d12, _d13, _d14, _d15, _d16, _d17) \
		{ (PWR2X(_d1) << 4 | (PWR2X(_d0) & 0x0f)) & 0xff, \
		  (PWR2X(_d3) << 4 | (PWR2X(_d2) & 0x0f)) & 0xff, \
		  (PWR2X(_d5) << 4 | (PWR2X(_d4) & 0x0f)) & 0xff, \
		  (PWR2X(_d7) << 4 | (PWR2X(_d6) & 0x0f)) & 0xff, \
		  (PWR2X(_d9) << 4 | (PWR2X(_d8) & 0x0f)) & 0xff, \
		  (PWR2X(_d11) << 4 | (PWR2X(_d10) & 0x0f)) & 0xff, \
		  (PWR2X(_d13) << 4 | (PWR2X(_d12) & 0x0f)) & 0xff, \
		  (PWR2X(_d15) << 4 | (PWR2X(_d14) & 0x0f)) & 0xff, \
		  (PWR2X(_d17) << 4 | (PWR2X(_d16) & 0x0f)) & 0xff }

static const struct qca9880_eeprom qca9880_cus223 = {
	.baseEepHeader = {
		.length = LE16CONST(sizeof(struct qca9880_eeprom)),
		.checksum = LE16CONST(0x9211),
		.eepromVersion = 2,
		.templateVersion = qca9880_tpl_ver_cus223,
		.macAddr = {0x00, 0x03, 0x07, 0x12, 0x34, 0x56},
		.opCapBrdFlags = {
			.opFlags = QCA9880_OPFLAGS_11A |
				   QCA9880_OPFLAGS_5G_HT40 |
				   QCA9880_OPFLAGS_5G_HT20,
			.featureFlags = QCA9880_FEATURE_TEMP_COMP |
					QCA9880_FEATURE_INT_REGULATOR,
			.__unkn_03 = 0x08,
			.boardFlags = LE32CONST(0x00080c44),
			.opFlags2 = QCA9880_OPFLAGS2_5G_VHT20 |
				    QCA9880_OPFLAGS2_5G_VHT40 |
				    QCA9880_OPFLAGS2_5G_VHT80,
		},
		.txrxMask = 0x77,
		.swreg = 0x98,
		.param_for_tuning_caps = 0x4a,
		.param_for_tuning_caps1 = 0x4a,
	},
	.modalHeader5G = {
		.xpaBiasLvl = 0x0c,
		.antennaGain = 1,
		.antCtrlCommon = LE32CONST(0x00000220),
		.antCtrlCommon2 = LE32CONST(0x00011111),
		.antCtrlChain = {
			LE16CONST(0x0010), LE16CONST(0x0010), LE16CONST(0x0010)
		},
	},
	.modalHeader2G = {
		.xpaBiasLvl = 0x0f,
		.antCtrlCommon = LE32CONST(0x00090449),
		.antCtrlCommon2 = LE32CONST(0x00099999),
		.antCtrlChain = {
			LE16CONST(0x0000), LE16CONST(0x0000), LE16CONST(0x0000)
		},
	},
	.baseExt = {
		.xatten1DB = {
			{0x00, 0x18, 0x18, 0x18},
			{0x00, 0x18, 0x18, 0x18},
			{0x00, 0x18, 0x18, 0x18}
		},
		.xatten1Margin = {
			{0x00, 0x08, 0x08, 0x08},
			{0x00, 0x08, 0x08, 0x08},
			{0x00, 0x08, 0x08, 0x08}
		},
	},
	.thermCal = {
		.thermAdcScaledGain = LE16CONST(0x00cd),
		.rbias = 0x40,
	},
	.calFreqPier2G = {0xff, 0xff, 0xff},
	.calPierData2G = {
		{ .thermCalVal = 121, .voltCalVal = 100 },
		{ .thermCalVal = 121, .voltCalVal = 100 },
		{ .thermCalVal = 121, .voltCalVal = 100 },
	},
	.targetFreqbin2GCck = {
		FREQ2FBIN(2412, 1), FREQ2FBIN(2472, 1)
	},
	.targetFreqbin2GLeg = {
		FREQ2FBIN(2412, 1), FREQ2FBIN(2442, 1), FREQ2FBIN(2472, 1)
	},
	.targetFreqbin2GVHT20 = {
		FREQ2FBIN(2412, 1), FREQ2FBIN(2442, 1), FREQ2FBIN(2472, 1)
	},
	.targetFreqbin2GVHT40 = {
		FREQ2FBIN(2412, 1), FREQ2FBIN(2442, 1), FREQ2FBIN(2472, 1)
	},
	.targetPower2GCck = {
		{ { PWR2X(14), PWR2X(14), PWR2X(14), PWR2X(14) } },
		{ { PWR2X(14), PWR2X(14), PWR2X(14), PWR2X(14) } }
	},
	.targetPower2GLeg = {
		{ { PWR2X(14), PWR2X(14), PWR2X(13), PWR2X(13) } },
		{ { PWR2X(14), PWR2X(14), PWR2X(13), PWR2X(13) } },
		{ { PWR2X(14), PWR2X(14), PWR2X(13), PWR2X(13) } },
	},
	.targetPower2GVHT20 = {
		{
			PWR2XVHTBASE(10, 10, 10),
			PWR2XVHTDELTA(4, 4, 2,
				      2, 0, 2, 4, 0,
				      2, 4, 2, 2, 0,
				      0, 0, 0, 0, 0)
		}, {
			PWR2XVHTBASE(10, 10, 10),
			PWR2XVHTDELTA(4, 4, 2,
				      2, 0, 2, 4, 0,
				      2, 4, 2, 2, 0,
				      0, 0, 0, 0, 0)
		}, {
			PWR2XVHTBASE(10, 10, 10),
			PWR2XVHTDELTA(4, 4, 2,
				      2, 0, 2, 4, 0,
				      2, 4, 2, 2, 0,
				      0, 0, 0, 0, 0)
		}
	},
	.targetPower2GVHT40 = {
		{
			PWR2XVHTBASE(10, 10, 10),
			PWR2XVHTDELTA(4, 4, 2,
				      2, 0, 2, 4, 0,
				      2, 4, 2, 2, 0,
				      0, 0, 0, 0, 0)
		}, {
			PWR2XVHTBASE(10, 10, 10),
			PWR2XVHTDELTA(4, 4, 2,
				      2, 0, 2, 4, 0,
				      2, 4, 2, 2, 0,
				      0, 0, 0, 0, 0)
		}, {
			PWR2XVHTBASE(10, 10, 10),
			PWR2XVHTDELTA(4, 4, 2,
				      2, 0, 2, 4, 0,
				      2, 4, 2, 2, 0,
				      0, 0, 0, 0, 0)
		}
	},
	.ctlIndex2G = {
		0x11, 0x12, 0x15, 0x17, 0x00, 0x00,
		0x41, 0x42, 0x45, 0x47, 0x00, 0x00,
		0x31, 0x32, 0x35, 0x37, 0x00, 0x00
	},
	.ctlFreqBin2G = {
		{
			FREQ2FBIN(2412, 1), FREQ2FBIN(2417, 1),
			FREQ2FBIN(2457, 1), FREQ2FBIN(2462, 1)
		}, {
			FREQ2FBIN(2412, 1), FREQ2FBIN(2417, 1),
			FREQ2FBIN(2462, 1), 0xff
		}, {
			FREQ2FBIN(2412, 1), FREQ2FBIN(2417, 1),
			FREQ2FBIN(2462, 1), 0xff
		}, {
			FREQ2FBIN(2422, 1), FREQ2FBIN(2427, 1),
			FREQ2FBIN(2447, 1), FREQ2FBIN(2452, 1)
		}, {
			0x00, 0x00, 0x00, 0x00
		}, {
			0x00, 0x00, 0x00, 0x00
		},

		{
			FREQ2FBIN(2412, 1), FREQ2FBIN(2417, 1),
			FREQ2FBIN(2472, 1), FREQ2FBIN(2484, 1)
		}, {
			FREQ2FBIN(2412, 1), FREQ2FBIN(2417, 1),
			FREQ2FBIN(2472, 1), 0x00,
		}, {
			FREQ2FBIN(2412, 1), FREQ2FBIN(2417, 1),
			FREQ2FBIN(2472, 1), 0x00,
		}, {
			FREQ2FBIN(2422, 1), FREQ2FBIN(2427, 1),
			FREQ2FBIN(2447, 1), FREQ2FBIN(2462, 1),
		}, {
			0x00, 0x00, 0x00, 0x00
		}, {
			0x00, 0x00, 0x00, 0x00
		},

		{
			FREQ2FBIN(2412, 1), FREQ2FBIN(2417, 1),
			FREQ2FBIN(2472, 1), 0x00,
		}, {
			FREQ2FBIN(2412, 1), FREQ2FBIN(2417, 1),
			FREQ2FBIN(2472, 1), 0x00,
		}, {
			FREQ2FBIN(2412, 1), FREQ2FBIN(2417, 1),
			FREQ2FBIN(2472, 1), 0x00,
		}, {
			FREQ2FBIN(2422, 1), FREQ2FBIN(2427, 1),
			FREQ2FBIN(2447, 1), FREQ2FBIN(2462, 1),
		}, {
			0x00, 0x00, 0x00, 0x00
		}, {
			0x00, 0x00, 0x00, 0x00
		}
	},
	.ctlData2G = {
		{
			CTLPACK(60, 0), CTLPACK(61, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(61, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(61, 0), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(61, 0)
		}, {
			CTLPACK(61, 0), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			0x00, 0x00, 0x00, 0x00
		}, {
			0x00, 0x00, 0x00, 0x00
		},

		{
			CTLPACK(60, 0), CTLPACK(61, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(61, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(61, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			0x00, 0x00, 0x00, 0x00
		}, {
			0x00, 0x00, 0x00, 0x00
		},

		{
			CTLPACK(60, 0), CTLPACK(61, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(61, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0)
		}, {
			0x00, 0x00, 0x00, 0x00
		}, {
			0x00, 0x00, 0x00, 0x00
		}
	},
	.alphaThermTbl2G = {
		{
			{0x1a, 0x1c, 0x1e, 0x20},
			{0x1a, 0x1c, 0x1e, 0x20},
			{0x1a, 0x1c, 0x1e, 0x20},
			{0x1a, 0x1c, 0x1e, 0x20}
		}, {
			{0x1a, 0x1c, 0x1e, 0x20},
			{0x1a, 0x1c, 0x1e, 0x20},
			{0x1a, 0x1c, 0x1e, 0x20},
			{0x1a, 0x1c, 0x1e, 0x20}
		}, {
			{0x1a, 0x1c, 0x1e, 0x20},
			{0x1a, 0x1c, 0x1e, 0x20},
			{0x1a, 0x1c, 0x1e, 0x20},
			{0x1a, 0x1c, 0x1e, 0x20}
		}
	},
	.calFreqPier5G = {
		FREQ2FBIN(5180, 0), FREQ2FBIN(5240, 0),
		FREQ2FBIN(5260, 0), FREQ2FBIN(5320, 0),
		FREQ2FBIN(5500, 0), FREQ2FBIN(5700, 0),
		FREQ2FBIN(5745, 0), FREQ2FBIN(5805, 0)
	},
	.calPierData5G = {
		{
			.calPerChain = {
				{
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x0097),
						LE16CONST(0x0097)
					}
				}, {
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x0085),
						LE16CONST(0x0085)
					}
				}, {
					.txgainIdx = {0x09, 0x0d},
					.power = {
						LE16CONST(0x0095),
						LE16CONST(0x0094)
					}
				}
			},
			.dacGain = {0x00, 0xf8},
			.thermCalVal = 0x78,
			.voltCalVal = 0x00,
		}, {
			.calPerChain = {
				{
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x009e),
						LE16CONST(0x009d)
					}
				}, {
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x008f),
						LE16CONST(0x008f)
					}
				}, {
					.txgainIdx = {0x09, 0x0d},
					.power = {
						LE16CONST(0x009b),
						LE16CONST(0x0099)
					}
				}
			},
			.dacGain = {0x00, 0xf8},
			.thermCalVal = 0x81,
			.voltCalVal = 0x62,
		}, {
			.calPerChain = {
				{
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x009f),
						LE16CONST(0x009d)
					}
				}, {
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x0092),
						LE16CONST(0x0092)
					}
				}, {
					.txgainIdx = {0x09, 0x0d},
					.power = {
						LE16CONST(0x009c),
						LE16CONST(0x009a)
					}
				}
			},
			.dacGain = {0x00, 0xf8},
			.thermCalVal = 0x81,
			.voltCalVal = 0x62,
		}, {
			.calPerChain = {
				{
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x00a1),
						LE16CONST(0x00a0)
					}
				}, {
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x0098),
						LE16CONST(0x0097)
					}
				}, {
					.txgainIdx = {0x09, 0x0d},
					.power = {
						LE16CONST(0x009c),
						LE16CONST(0x009a)
					}
				}
			},
			.dacGain = {0x00, 0xf8},
			.thermCalVal = 0x81,
			.voltCalVal = 0x62,
		}, {
			.calPerChain = {
				{
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x00a2),
						LE16CONST(0x009f)
					}
				}, {
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x009b),
						LE16CONST(0x0098)
					}
				}, {
					.txgainIdx = {0x09, 0x0d},
					.power = {
						LE16CONST(0x009a),
						LE16CONST(0x0096)
					}
				}
			},
			.dacGain = {0x00, 0xf8},
			.thermCalVal = 0x81,
			.voltCalVal = 0x62,
		}, {
			.calPerChain = {
				{
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x0097),
						LE16CONST(0x0092)
					}
				}, {
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x0096),
						LE16CONST(0x0091)
					}
				}, {
					.txgainIdx = {0x09, 0x0d},
					.power = {
						LE16CONST(0x008d),
						LE16CONST(0x0089)
					}
				}
			},
			.dacGain = {0x00, 0xf8},
			.thermCalVal = 0x81,
			.voltCalVal = 0x62,
		}, {
			.calPerChain = {
				{
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x0092),
						LE16CONST(0x008d)
					}
				}, {
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x0094),
						LE16CONST(0x008f)
					}
				}, {
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x009b),
						LE16CONST(0x0094)
					}
				}
			},
			.dacGain = {0x00, 0xf8},
			.thermCalVal = 0x81,
			.voltCalVal = 0x62,
		}, {
			.calPerChain = {
				{
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x008f),
						LE16CONST(0x008a)
					}
				}, {
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x0090),
						LE16CONST(0x008c)
					}
				}, {
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x0095),
						LE16CONST(0x008e)
					}
				}
			},
			.dacGain = {0x00, 0xf8},
			.thermCalVal = 0x81,
			.voltCalVal = 0x62,
		},
	},
	.targetFreqbin5GLeg = {
		FREQ2FBIN(5180, 0), FREQ2FBIN(5240, 0), FREQ2FBIN(5320, 0),
		FREQ2FBIN(5500, 0), FREQ2FBIN(5600, 0), FREQ2FBIN(5745, 0)
	},
	.targetFreqbin5GVHT20 = {
		FREQ2FBIN(5180, 0), FREQ2FBIN(5240, 0), FREQ2FBIN(5320, 0),
		FREQ2FBIN(5500, 0), FREQ2FBIN(5600, 0), FREQ2FBIN(5745, 0)
	},
	.targetFreqbin5GVHT40 = {
		FREQ2FBIN(5180, 0), FREQ2FBIN(5240, 0), FREQ2FBIN(5320, 0),
		FREQ2FBIN(5500, 0), FREQ2FBIN(5600, 0), FREQ2FBIN(5745, 0)
	},
	.targetFreqbin5GVHT80 = {
		FREQ2FBIN(5180, 0), FREQ2FBIN(5240, 0), FREQ2FBIN(5320, 0),
		FREQ2FBIN(5500, 0), FREQ2FBIN(5600, 0), FREQ2FBIN(5745, 0)
	},
	.targetPower5GLeg = {
		{ { PWR2X(20), PWR2X(20), PWR2X(20), PWR2X(18) } },
		{ { PWR2X(20), PWR2X(20), PWR2X(20), PWR2X(18) } },
		{ { PWR2X(20), PWR2X(20), PWR2X(20), PWR2X(18) } },
		{ { PWR2X(20), PWR2X(20), PWR2X(20), PWR2X(18) } },
		{ { PWR2X(20), PWR2X(20), PWR2X(20), PWR2X(18) } },
		{ { PWR2X(20), PWR2X(20), PWR2X(20), PWR2X(18) } },
	},
	.targetPower5GVHT20 = {
		{
			PWR2XVHTBASE(13, 13, 13),
			PWR2XVHTDELTA(7, 7, 7,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0)
		}, {
			PWR2XVHTBASE(13, 13, 13),
			PWR2XVHTDELTA(7, 7, 7,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0)
		}, {
			PWR2XVHTBASE(13, 13, 13),
			PWR2XVHTDELTA(7, 7, 7,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0)
		}, {
			PWR2XVHTBASE(13, 13, 13),
			PWR2XVHTDELTA(7, 7, 7,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0)
		}, {
			PWR2XVHTBASE(13, 13, 13),
			PWR2XVHTDELTA(7, 7, 7,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0)
		}, {
			PWR2XVHTBASE(13, 13, 13),
			PWR2XVHTDELTA(7, 7, 7,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0)
		}
	},
	.targetPower5GVHT40 = {
		{
			PWR2XVHTBASE(13, 13, 13),
			PWR2XVHTDELTA(7, 7, 7,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0)
		}, {
			PWR2XVHTBASE(13, 13, 13),
			PWR2XVHTDELTA(7, 7, 7,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0)
		}, {
			PWR2XVHTBASE(13, 13, 13),
			PWR2XVHTDELTA(7, 7, 7,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0)
		}, {
			PWR2XVHTBASE(13, 13, 13),
			PWR2XVHTDELTA(7, 7, 7,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0)
		}, {
			PWR2XVHTBASE(13, 13, 13),
			PWR2XVHTDELTA(7, 7, 7,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0)
		}, {
			PWR2XVHTBASE(13, 13, 13),
			PWR2XVHTDELTA(7, 7, 7,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0)
		}
	},
	.targetPower5GVHT80 = {
		{
			PWR2XVHTBASE(13, 13, 13),
			PWR2XVHTDELTA(7, 7, 7,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0)
		}, {
			PWR2XVHTBASE(13, 13, 13),
			PWR2XVHTDELTA(7, 7, 7,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0)
		}, {
			PWR2XVHTBASE(13, 13, 13),
			PWR2XVHTDELTA(7, 7, 7,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0)
		}, {
			PWR2XVHTBASE(13, 13, 13),
			PWR2XVHTDELTA(7, 7, 7,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0)
		}, {
			PWR2XVHTBASE(13, 13, 13),
			PWR2XVHTDELTA(7, 7, 7,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0)
		}, {
			PWR2XVHTBASE(13, 13, 13),
			PWR2XVHTDELTA(7, 7, 7,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0,
				      7, 7, 5, 3, 0)
		}
	},
	.ctlIndex5G = {
		0x10, 0x16, 0x18, 0x19, 0x00, 0x00,
		0x40, 0x46, 0x48, 0x49, 0x00, 0x00,
		0x30, 0x36, 0x38, 0x39, 0x00, 0x00
	},
	.ctlFreqBin5G = {
		{
			FREQ2FBIN(5180, 0), FREQ2FBIN(5260, 0),
			FREQ2FBIN(5280, 0), FREQ2FBIN(5500, 0),
			FREQ2FBIN(5600, 0), FREQ2FBIN(5700, 0),
			FREQ2FBIN(5745, 0), FREQ2FBIN(5825, 0)
		}, {
			FREQ2FBIN(5180, 0), FREQ2FBIN(5260, 0),
			FREQ2FBIN(5280, 0), FREQ2FBIN(5500, 0),
			FREQ2FBIN(5520, 0), FREQ2FBIN(5700, 0),
			FREQ2FBIN(5745, 0), FREQ2FBIN(5825, 0)
		}, {
			FREQ2FBIN(5190, 0), FREQ2FBIN(5230, 0),
			FREQ2FBIN(5270, 0), FREQ2FBIN(5310, 0),
			FREQ2FBIN(5510, 0), FREQ2FBIN(5550, 0),
			FREQ2FBIN(5670, 0), FREQ2FBIN(5755, 0)
		}, {
			FREQ2FBIN(5180, 0), FREQ2FBIN(5260, 0),
			FREQ2FBIN(5280, 0), FREQ2FBIN(5500, 0),
			FREQ2FBIN(5520, 0), FREQ2FBIN(5700, 0),
			FREQ2FBIN(5745, 0), FREQ2FBIN(5825, 0)
		}, {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		}, {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		},

		{
			FREQ2FBIN(5180, 0), FREQ2FBIN(5200, 0),
			FREQ2FBIN(5260, 0), FREQ2FBIN(5320, 0),
			FREQ2FBIN(5500, 0), FREQ2FBIN(5700, 0),
			0xff, 0xff
		}, {
			FREQ2FBIN(5180, 0), FREQ2FBIN(5260, 0),
			FREQ2FBIN(5500, 0), FREQ2FBIN(5700, 0),
			0xff, 0xff, 0xff, 0xff
		}, {
			FREQ2FBIN(5190, 0), FREQ2FBIN(5270, 0),
			FREQ2FBIN(5310, 0), FREQ2FBIN(5510, 0),
			FREQ2FBIN(5590, 0), FREQ2FBIN(5670, 0),
			0xff, 0xff
		}, {
			FREQ2FBIN(5180, 0), FREQ2FBIN(5260, 0),
			FREQ2FBIN(5500, 0), FREQ2FBIN(5700, 0),
			0xff, 0xff, 0xff, 0xff
		}, {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		}, {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		},

		{
			FREQ2FBIN(5180, 0), FREQ2FBIN(5200, 0),
			FREQ2FBIN(5220, 0), FREQ2FBIN(5260, 0),
			FREQ2FBIN(5500, 0), FREQ2FBIN(5600, 0),
			FREQ2FBIN(5700, 0), FREQ2FBIN(5745, 0)
		}, {
			FREQ2FBIN(5180, 0), FREQ2FBIN(5260, 0),
			FREQ2FBIN(5320, 0), FREQ2FBIN(5500, 0),
			FREQ2FBIN(5560, 0), FREQ2FBIN(5700, 0),
			FREQ2FBIN(5745, 0), FREQ2FBIN(5825, 0)
		}, {
			FREQ2FBIN(5190, 0), FREQ2FBIN(5230, 0),
			FREQ2FBIN(5270, 0), FREQ2FBIN(5510, 0),
			FREQ2FBIN(5550, 0), FREQ2FBIN(5670, 0),
			FREQ2FBIN(5755, 0), FREQ2FBIN(5795, 0)
		}, {
			FREQ2FBIN(5180, 0), FREQ2FBIN(5260, 0),
			FREQ2FBIN(5320, 0), FREQ2FBIN(5500, 0),
			FREQ2FBIN(5560, 0), FREQ2FBIN(5700, 0),
			FREQ2FBIN(5745, 0), FREQ2FBIN(5825, 0)
		}, {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		}, {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		}
	},
	.ctlData5G = {
		{
			CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0),
			CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0),
			CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(61, 0), CTLPACK(60, 0), CTLPACK(61, 0),
			CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0)
		}, {
			CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0),
			CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(60, 0)
		}, {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		}, {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		},

		{
			CTLPACK(60, 0), CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(60, 0),
			CTLPACK(61, 0), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(60, 0),
			CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0),
			CTLPACK(61, 0), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(60, 0),
			CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		}, {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		},

		{
			CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0),
			CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0)
		}, {
			CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(60, 0), CTLPACK(61, 0),
			CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(61, 0), CTLPACK(60, 0), CTLPACK(61, 0), CTLPACK(61, 0),
			CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(60, 0), CTLPACK(61, 0)
		}, {
			CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(60, 0), CTLPACK(61, 0),
			CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(61, 0), CTLPACK(60, 0)
		}, {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		}, {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		}
	},
	.alphaThermTbl5G = {
		{
			{0x22, 0x23, 0x25, 0x2a}, {0x21, 0x26, 0x2b, 0x2f},
			{0x2c, 0x2d, 0x31, 0x39}, {0x2c, 0x30, 0x33, 0x36},
			{0x2c, 0x31, 0x32, 0x36}, {0x2c, 0x2f, 0x33, 0x37},
			{0x2a, 0x2d, 0x33, 0x33}, {0x27, 0x2e, 0x31, 0x35}
		}, {
			{0x22, 0x22, 0x27, 0x2a}, {0x22, 0x27, 0x2b, 0x2e},
			{0x2a, 0x2d, 0x31, 0x33}, {0x2a, 0x2f, 0x32, 0x34},
			{0x2f, 0x32, 0x32, 0x35}, {0x2f, 0x31, 0x32, 0x36},
			{0x29, 0x30, 0x35, 0x35}, {0x2c, 0x32, 0x34, 0x37}
		}, {
			{0x24, 0x22, 0x25, 0x28}, {0x22, 0x26, 0x29, 0x2c},
			{0x2a, 0x2d, 0x2f, 0x31}, {0x2f, 0x30, 0x30, 0x31},
			{0x2f, 0x30, 0x2f, 0x32}, {0x2d, 0x2d, 0x2e, 0x32},
			{0x28, 0x2b, 0x2e, 0x30}, {0x24, 0x2b, 0x2c, 0x2f}
		}
	},
};

static const struct qca9880_eeprom qca9880_xb140 = {
	.baseEepHeader = {
		.length = LE16CONST(sizeof(struct qca9880_eeprom)),
		.checksum = LE16CONST(0x623e),
		.eepromVersion = 2,
		.templateVersion = qca9880_tpl_ver_xb140,
		.macAddr = {0x00, 0x03, 0x7f, 0x00, 0x00, 0x00},
		.opCapBrdFlags = {
			.opFlags = QCA9880_OPFLAGS_11A |
				   QCA9880_OPFLAGS_11G |
				   QCA9880_OPFLAGS_5G_HT40 |
				   QCA9880_OPFLAGS_2G_HT40 |
				   QCA9880_OPFLAGS_5G_HT20 |
				   QCA9880_OPFLAGS_2G_HT20,
			.featureFlags = QCA9880_FEATURE_TEMP_COMP |
					QCA9880_FEATURE_INT_REGULATOR |
					QCA9880_FEATURE_TUNING_CAPS,
			.boardFlags = LE32CONST(0x00080c44),
			.opFlags2 = QCA9880_OPFLAGS2_5G_VHT20 |
				    QCA9880_OPFLAGS2_2G_VHT20 |
				    QCA9880_OPFLAGS2_5G_VHT40 |
				    QCA9880_OPFLAGS2_2G_VHT40 |
				    QCA9880_OPFLAGS2_5G_VHT80,
		},
		.txrxMask = 0x77,
		.swreg = 0x98,
		.param_for_tuning_caps = 0x4e,
		.custData = {"xb140-012-n0303"},
		.param_for_tuning_caps1 = 0x4e,
	},
	.modalHeader5G = {
		.xpaBiasLvl = 0x1f,
		.antCtrlCommon = LE32CONST(0x00000220),
		.antCtrlCommon2 = LE32CONST(0x000ddddd),
		.antCtrlChain = {
			LE16CONST(0x0010), LE16CONST(0x0010), LE16CONST(0x0010)
		},
	},
	.modalHeader2G = {
		.xpaBiasLvl = 0x1c,
		.antennaGain = 3,
		.antCtrlCommon = LE32CONST(0x00000220),
		.antCtrlCommon2 = LE32CONST(0x00011111),
		.antCtrlChain = {
			LE16CONST(0x0010), LE16CONST(0x0010), LE16CONST(0x0010)
		},
	},
	.baseExt = {
		.xatten1DB = {
			{0x20, 0x1b, 0x1c, 0x1c},
			{0x1f, 0x1c, 0x19, 0x1b},
			{0x20, 0x1c, 0x1d, 0x1c}
		},
		.xatten1Margin = {
			{0x0a, 0x0a, 0x0a, 0x0a},
			{0x0a, 0x0a, 0x0a, 0x0a},
			{0x0a, 0x0a, 0x0a, 0x0a}
		},
	},
	.thermCal = {
		.thermAdcScaledGain = LE16CONST(0x00cd),
		.rbias = 0x40,
	},
	.calFreqPier2G = {
		FREQ2FBIN(2412, 1), FREQ2FBIN(2437, 1), FREQ2FBIN(2462, 1)
	},
	.calPierData2G = {
		{
			.calPerChain = {
				{
					.txgainIdx = {0x08, 0x0c},
					.power = {
						LE16CONST(0x0064),
						LE16CONST(0x0061)
					}
				}, {
					.txgainIdx = {0x09, 0x0d},
					.power = {
						LE16CONST(0x006a),
						LE16CONST(0x0069)
					}
				}, {
					.txgainIdx = {0x09, 0x0d},
					.power = {
						LE16CONST(0x0065),
						LE16CONST(0x0062)
					}
				}
			},
			.dacGain = {0x00, 0xf8},
			.thermCalVal = 0x81,
			.voltCalVal = 0x00,
		}, {
			.calPerChain = {
				{
					.txgainIdx = {0x08, 0x0c},
					.power = {
						LE16CONST(0x0068),
						LE16CONST(0x0066)
					}
				}, {
					.txgainIdx = {0x09, 0x0d},
					.power = {
						LE16CONST(0x006c),
						LE16CONST(0x006a)
					}
				}, {
					.txgainIdx = {0x09, 0x0d},
					.power = {
						LE16CONST(0x0067),
						LE16CONST(0x0064)
					}
				}
			},
			.dacGain = {0x00, 0xf8},
			.thermCalVal = 0x81,
			.voltCalVal = 0x00,
		}, {
			.calPerChain = {
				{
					.txgainIdx = {0x08, 0x0c},
					.power = {
						LE16CONST(0x0068),
						LE16CONST(0x0068)
					}
				}, {
					.txgainIdx = {0x09, 0x0d},
					.power = {
						LE16CONST(0x006e),
						LE16CONST(0x006a)
					}
				}, {
					.txgainIdx = {0x09, 0x0d},
					.power = {
						LE16CONST(0x0067),
						LE16CONST(0x0064)
					}
				}
			},
			.dacGain = {0x00, 0xf8},
			.thermCalVal = 0x81,
			.voltCalVal = 0x00,
		}
	},
	.extTPow2xDelta2G = {
		0x0f, 0x21, 0x3c, 0x84, 0xf0, 0x10, 0xc2, 0x43,
		0x08, 0x0f, 0x21, 0x3c, 0x84, 0x00,
	},
	.targetFreqbin2GCck = {
		FREQ2FBIN(2412, 1), FREQ2FBIN(2472, 1)
	},
	.targetFreqbin2GLeg = {
		FREQ2FBIN(2412, 1), FREQ2FBIN(2437, 1), FREQ2FBIN(2472, 1)
	},
	.targetFreqbin2GVHT20 = {
		FREQ2FBIN(2412, 1), FREQ2FBIN(2437, 1), FREQ2FBIN(2472, 1)
	},
	.targetFreqbin2GVHT40 = {
		FREQ2FBIN(2412, 1), FREQ2FBIN(2437, 1), FREQ2FBIN(2472, 1)
	},
	.targetPower2GCck = {
		{ { PWR2X(18), PWR2X(18), PWR2X(18), PWR2X(18) } },
		{ { PWR2X(18), PWR2X(18), PWR2X(18), PWR2X(18) } }
	},
	.targetPower2GLeg = {
		{ { PWR2X(19), PWR2X(17.5), PWR2X(16.5), PWR2X(15) } },
		{ { PWR2X(19), PWR2X(17.5), PWR2X(16.5), PWR2X(15) } },
		{ { PWR2X(19), PWR2X(17.5), PWR2X(16.5), PWR2X(15) } }
	},
	.targetPower2GVHT20 = {
		/**
		 * NB: We store here only 4 LSB of power delta. Full delta
		 * values are provided here only for reference. In fact the 5th
		 * high bit of each delta value will be truncated. 5th (ext) bit
		 * actually stored in the extTPow2xDelta2G field.
		 */
		{
			PWR2XVHTBASE(9, 9, 9),
			PWR2XVHTDELTA(10, 10, 9,
				      9, 6, 4, 2, 0,
				      9, 6, 4, 2, 0,
				      9, 6, 4, 2, 0)
		}, {
			PWR2XVHTBASE(9, 9, 9),
			PWR2XVHTDELTA(10, 10, 9,
				      9, 6, 4, 2, 0,
				      9, 6, 4, 2, 0,
				      9, 6, 4, 2, 0)
		}, {
			PWR2XVHTBASE(9, 9, 9),
			PWR2XVHTDELTA(10, 10, 9,
				      9, 6, 4, 2, 0,
				      9, 6, 4, 2, 0,
				      9, 6, 4, 2, 0)
		}
	},
	.targetPower2GVHT40 = {
		/**
		 * NB: We store here only 4 LSB of power delta. Full delta
		 * values are provided here only for reference. In fact the 5th
		 * high bit of each delta value will be truncated. 5th (ext) bit
		 * actually stored in the extTPow2xDelta2G field.
		 */
		{
			PWR2XVHTBASE(8, 8, 8),
			PWR2XVHTDELTA(10, 10, 9,
				      9, 7, 5, 2, 0,
				      9, 7, 5, 2, 0,
				      9, 7, 5, 2, 0)
		}, {
			PWR2XVHTBASE(8, 8, 8),
			PWR2XVHTDELTA(10, 10, 9,
				      9, 7, 5, 2, 0,
				      9, 7, 5, 2, 0,
				      9, 7, 5, 2, 0)
		}, {
			PWR2XVHTBASE(8, 8, 8),
			PWR2XVHTDELTA(10, 10, 9,
				      9, 7, 5, 2, 0,
				      9, 7, 5, 2, 0,
				      9, 7, 5, 2, 0)
		}
	},
	.ctlIndex2G = {
		0x11, 0x12, 0x15, 0x17, 0x1a, 0x1c,
		0x41, 0x42, 0x45, 0x47, 0x4a, 0x4c,
		0x31, 0x32, 0x35, 0x37, 0x3a, 0x3c
	},
	.ctlFreqBin2G = {
		{
			FREQ2FBIN(2412, 1), FREQ2FBIN(2417, 1),
			FREQ2FBIN(2457, 1), FREQ2FBIN(2462, 1)
		}, {
			FREQ2FBIN(2412, 1), FREQ2FBIN(2417, 1),
			FREQ2FBIN(2457, 1), FREQ2FBIN(2462, 1)
		}, {
			FREQ2FBIN(2412, 1), FREQ2FBIN(2417, 1),
			FREQ2FBIN(2457, 1), FREQ2FBIN(2462, 1)
		}, {
			FREQ2FBIN(2422, 1), FREQ2FBIN(2427, 1),
			FREQ2FBIN(2447, 1), FREQ2FBIN(2452, 1)
		}, {
			FREQ2FBIN(2412, 1), FREQ2FBIN(2417, 1),
			FREQ2FBIN(2457, 1), FREQ2FBIN(2462, 1)
		}, {
			FREQ2FBIN(2422, 1), FREQ2FBIN(2427, 1),
			FREQ2FBIN(2447, 1), FREQ2FBIN(2452, 1)
		},

		{
			FREQ2FBIN(2412, 1), FREQ2FBIN(2417, 1),
			FREQ2FBIN(2472, 1), FREQ2FBIN(2484, 1)
		}, {
			FREQ2FBIN(2412, 1), FREQ2FBIN(2417, 1),
			FREQ2FBIN(2467, 1), FREQ2FBIN(2472, 1)
		}, {
			FREQ2FBIN(2412, 1), FREQ2FBIN(2417, 1),
			FREQ2FBIN(2467, 1), FREQ2FBIN(2472, 1)
		}, {
			FREQ2FBIN(2422, 1), FREQ2FBIN(2427, 1),
			FREQ2FBIN(2457, 1), FREQ2FBIN(2462, 1)
		}, {
			FREQ2FBIN(2412, 1), FREQ2FBIN(2417, 1),
			FREQ2FBIN(2467, 1), FREQ2FBIN(2472, 1)
		}, {
			FREQ2FBIN(2422, 1), FREQ2FBIN(2427, 1),
			FREQ2FBIN(2457, 1), FREQ2FBIN(2462, 1)
		},

		{
			FREQ2FBIN(2412, 1), FREQ2FBIN(2417, 1),
			FREQ2FBIN(2467, 1), FREQ2FBIN(2472, 1)
		}, {
			FREQ2FBIN(2412, 1), FREQ2FBIN(2417, 1),
			FREQ2FBIN(2467, 1), FREQ2FBIN(2472, 1)
		}, {
			FREQ2FBIN(2412, 1), FREQ2FBIN(2417, 1),
			FREQ2FBIN(2467, 1), FREQ2FBIN(2472, 1)
		}, {
			FREQ2FBIN(2422, 1), FREQ2FBIN(2427, 1),
			FREQ2FBIN(2457, 1), FREQ2FBIN(2462, 1)
		}, {
			FREQ2FBIN(2412, 1), FREQ2FBIN(2417, 1),
			FREQ2FBIN(2467, 1), FREQ2FBIN(2472, 1)
		}, {
			FREQ2FBIN(2422, 1), FREQ2FBIN(2427, 1),
			FREQ2FBIN(2457, 1), FREQ2FBIN(2462, 1)
		}
	},
	.ctlData2G = {
		{
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		},

		{
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		},

		{
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		}
	},
	.alphaThermTbl2G = {
		{
			{0x17, 0x17, 0x1e, 0x21},
			{0x1a, 0x1a, 0x21, 0x24},
			{0x1c, 0x1c, 0x1f, 0x24},
			{0x1c, 0x1c, 0x1f, 0x24}
		}, {
			{0x1d, 0x1d, 0x23, 0x25},
			{0x20, 0x20, 0x25, 0x26},
			{0x24, 0x24, 0x24, 0x26},
			{0x24, 0x24, 0x24, 0x26}
		}, {
			{0x1d, 0x1d, 0x29, 0x24},
			{0x21, 0x21, 0x26, 0x25},
			{0x24, 0x24, 0x26, 0x25},
			{0x24, 0x24, 0x26, 0x25}
		}
	},
	.calFreqPier5G = {
		FREQ2FBIN(5200, 0), FREQ2FBIN(5300, 0),
		FREQ2FBIN(5520, 0), FREQ2FBIN(5580, 0),
		FREQ2FBIN(5640, 0), FREQ2FBIN(5700, 0),
		FREQ2FBIN(5765, 0), FREQ2FBIN(5805, 0)
	},
	.calPierData5G = {
		{
			.calPerChain = {
				{
					.txgainIdx = {0x08, 0x0c},
					.power = {
						LE16CONST(0x0077),
						LE16CONST(0x0082)
					}
				}, {
					.txgainIdx = {0x09, 0x0d},
					.power = {
						LE16CONST(0x0075),
						LE16CONST(0x0090)
					}
				}, {
					.txgainIdx = {0x08, 0x0c},
					.power = {
						LE16CONST(0x007d),
						LE16CONST(0x0089)
					}
				}
			},
			.dacGain = {0x00, 0xfa},
			.thermCalVal = 0x80,
			.voltCalVal = 0x00,
		}, {
			.calPerChain = {
				{
					.txgainIdx = {0x07, 0x0b},
					.power = {
						LE16CONST(0x006d),
						LE16CONST(0x007f)
					}
				}, {
					.txgainIdx = {0x08, 0x0c},
					.power = {
						LE16CONST(0x007c),
						LE16CONST(0x0082)
					}
				}, {
					.txgainIdx = {0x07, 0x0b},
					.power = {
						LE16CONST(0x0075),
						LE16CONST(0x0086)
					}
				}
			},
			.dacGain = {0x00, 0xfa},
			.thermCalVal = 0x81,
			.voltCalVal = 0x00,
		}, {
			.calPerChain = {
				{
					.txgainIdx = {0x08, 0x0c},
					.power = {
						LE16CONST(0x0074),
						LE16CONST(0x0081)
					}
				}, {
					.txgainIdx = {0x08, 0x0c},
					.power = {
						LE16CONST(0x007a),
						LE16CONST(0x0087)
					}
				}, {
					.txgainIdx = {0x08, 0x0c},
					.power = {
						LE16CONST(0x0075),
						LE16CONST(0x0083)
					}
				}
			},
			.dacGain = {0x00, 0xfa},
			.thermCalVal = 0x7f,
			.voltCalVal = 0x00,
		}, {
			.calPerChain = {
				{
					.txgainIdx = {0x09, 0x0d},
					.power = {
						LE16CONST(0x0077),
						LE16CONST(0x008c)
					}
				}, {
					.txgainIdx = {0x08, 0x0c},
					.power = {
						LE16CONST(0x0073),
						LE16CONST(0x007f)
					}
				}, {
					.txgainIdx = {0x08, 0x0c},
					.power = {
						LE16CONST(0x006e),
						LE16CONST(0x007b)
					}
				}
			},
			.dacGain = {0x00, 0xfa},
			.thermCalVal = 0x7f,
			.voltCalVal = 0x00,
		}, {
			.calPerChain = {
				{
					.txgainIdx = {0x09, 0x0c},
					.power = {
						LE16CONST(0x006c),
						LE16CONST(0x006c)
					}
				}, {
					.txgainIdx = {0x08, 0x0c},
					.power = {
						LE16CONST(0x006e),
						LE16CONST(0x0079)
					}
				}, {
					.txgainIdx = {0x09, 0x0c},
					.power = {
						LE16CONST(0x0073),
						LE16CONST(0x0071)
					}
				}
			},
			.dacGain = {0x00, 0xfa},
			.thermCalVal = 0x80,
			.voltCalVal = 0x00,
		}, {
			.calPerChain = {
				{
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x0076),
						LE16CONST(0x0089)
					}
				}, {
					.txgainIdx = {0x09, 0x0c},
					.power = {
						LE16CONST(0x0073),
						LE16CONST(0x0070)
					}
				}, {
					.txgainIdx = {0x09, 0x0c},
					.power = {
						LE16CONST(0x006d),
						LE16CONST(0x006c)
					}
				}
			},
			.dacGain = {0x00, 0xfa},
			.thermCalVal = 0x80,
			.voltCalVal = 0x00,
		}, {
			.calPerChain = {
				{
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x006b),
						LE16CONST(0x007f)
					}
				}, {
					.txgainIdx = {0x09, 0x0d},
					.power = {
						LE16CONST(0x0069),
						LE16CONST(0x007d)
					}
				}, {
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x0074),
						LE16CONST(0x0084)
					}
				}
			},
			.dacGain = {0x00, 0xfa},
			.thermCalVal = 0x81,
			.voltCalVal = 0x00,
		}, {
			.calPerChain = {
				{
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x006a),
						LE16CONST(0x007f)
					}
				}, {
					.txgainIdx = {0x0a, 0x0e},
					.power = {
						LE16CONST(0x0075),
						LE16CONST(0x0087)
					}
				}, {
					.txgainIdx = {0x0a, 0x0d},
					.power = {
						LE16CONST(0x0073),
						LE16CONST(0x0078)
					}
				}
			},
			.dacGain = {0x00, 0xfa},
			.thermCalVal = 0x80,
			.voltCalVal = 0x00,
		},
	},
	.extTPow2xDelta5G = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x03, 0x00, 0x0c, 0x00, 0x30,
		0x00, 0xc0, 0x00, 0x00, 0x03, 0x00, 0x0c, 0x00,
		0x00
	},
	.targetFreqbin5GLeg = {
		FREQ2FBIN(5180, 0), FREQ2FBIN(5220, 0), FREQ2FBIN(5320, 0),
		FREQ2FBIN(5500, 0), FREQ2FBIN(5600, 0), FREQ2FBIN(5725, 0)
	},
	.targetFreqbin5GVHT20 = {
		FREQ2FBIN(5180, 0), FREQ2FBIN(5220, 0), FREQ2FBIN(5320, 0),
		FREQ2FBIN(5500, 0), FREQ2FBIN(5600, 0), FREQ2FBIN(5725, 0)
	},
	.targetFreqbin5GVHT40 = {
		FREQ2FBIN(5180, 0), FREQ2FBIN(5220, 0), FREQ2FBIN(5320, 0),
		FREQ2FBIN(5500, 0), FREQ2FBIN(5600, 0), FREQ2FBIN(5725, 0)
	},
	.targetFreqbin5GVHT80 = {
		FREQ2FBIN(5180, 0), FREQ2FBIN(5220, 0), FREQ2FBIN(5320, 0),
		FREQ2FBIN(5500, 0), FREQ2FBIN(5600, 0), FREQ2FBIN(5725, 0)
	},
	.targetPower5GLeg = {
		{ { PWR2X(18), PWR2X(17), PWR2X(16), PWR2X(15) } },
		{ { PWR2X(18), PWR2X(17), PWR2X(16), PWR2X(15) } },
		{ { PWR2X(18), PWR2X(17), PWR2X(16), PWR2X(15) } },
		{ { PWR2X(18), PWR2X(17), PWR2X(16), PWR2X(15) } },
		{ { PWR2X(18), PWR2X(17), PWR2X(16), PWR2X(15) } },
		{ { PWR2X(18), PWR2X(17), PWR2X(16), PWR2X(15) } }
	},
	.targetPower5GVHT20 = {
		/**
		 * NB: We store here only 4 LSB of power delta. Full delta
		 * values are provided here only for reference. In fact the 5th
		 * high bit of each delta value will be truncated. 5th (ext) bit
		 * actually stored in the extTPow2xDelta5G field.
		 */
		{
			PWR2XVHTBASE(12, 12, 12),
			PWR2XVHTDELTA(6, 6, 5,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0)
		}, {
			PWR2XVHTBASE(12, 12, 12),
			PWR2XVHTDELTA(6, 6, 5,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0)
		}, {
			PWR2XVHTBASE(12, 12, 12),
			PWR2XVHTDELTA(6, 6, 5,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0)
		}, {
			PWR2XVHTBASE(12, 12, 12),
			PWR2XVHTDELTA(6, 6, 5,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0)
		}, {
			PWR2XVHTBASE(12, 12, 12),
			PWR2XVHTDELTA(6, 6, 5,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0)
		}, {
			PWR2XVHTBASE(12, 12, 12),
			PWR2XVHTDELTA(6, 6, 5,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0)
		}
	},
	.targetPower5GVHT40 = {
		/**
		 * NB: We store here only 4 LSB of power delta. Full delta
		 * values are provided here only for reference. In fact the 5th
		 * high bit of each delta value will be truncated. 5th (ext) bit
		 * actually stored in the extTPow2xDelta5G field.
		 */
		{
			PWR2XVHTBASE(11, 11, 11),
			PWR2XVHTDELTA(7, 7, 5,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0)
		}, {
			PWR2XVHTBASE(11, 11, 11),
			PWR2XVHTDELTA(7, 7, 5,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0)
		}, {
			PWR2XVHTBASE(11, 11, 11),
			PWR2XVHTDELTA(7, 7, 5,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0)
		}, {
			PWR2XVHTBASE(11, 11, 11),
			PWR2XVHTDELTA(7, 7, 5,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0)
		}, {
			PWR2XVHTBASE(11, 11, 11),
			PWR2XVHTDELTA(7, 7, 5,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0)
		}, {
			PWR2XVHTBASE(11, 11, 11),
			PWR2XVHTDELTA(7, 7, 5,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0)
		}
	},
	.targetPower5GVHT80 = {
		/**
		 * NB: We store here only 4 LSB of power delta. Full delta
		 * values are provided here only for reference. In fact the 5th
		 * high bit of each delta value will be truncated. 5th (ext) bit
		 * actually stored in the extTPow2xDelta5G field.
		 */
		{
			PWR2XVHTBASE(10, 10, 10),
			PWR2XVHTDELTA(8, 8, 5,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0)
		}, {
			PWR2XVHTBASE(10, 10, 10),
			PWR2XVHTDELTA(8, 8, 5,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0)
		}, {
			PWR2XVHTBASE(10, 10, 10),
			PWR2XVHTDELTA(8, 8, 5,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0)
		}, {
			PWR2XVHTBASE(10, 10, 10),
			PWR2XVHTDELTA(8, 8, 5,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0)
		}, {
			PWR2XVHTBASE(10, 10, 10),
			PWR2XVHTDELTA(8, 8, 5,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0)
		}, {
			PWR2XVHTBASE(10, 10, 10),
			PWR2XVHTDELTA(8, 8, 5,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0,
				      2, 1, 1, 0, 0)
		}
	},
	.ctlIndex5G = {
		0x10, 0x16, 0x18, 0x19, 0x1b, 0x1d,
		0x40, 0x46, 0x48, 0x49, 0x4b, 0x4d,
		0x30, 0x36, 0x38, 0x39, 0x3b, 0x3d
	},
	.ctlFreqBin5G = {
		{
			FREQ2FBIN(5180, 0), FREQ2FBIN(5260, 0),
			FREQ2FBIN(5280, 0), FREQ2FBIN(5500, 0),
			FREQ2FBIN(5700, 0), FREQ2FBIN(5720, 0),
			FREQ2FBIN(5745, 0), FREQ2FBIN(5825, 0)
		}, {
			FREQ2FBIN(5180, 0), FREQ2FBIN(5260, 0),
			FREQ2FBIN(5280, 0), FREQ2FBIN(5500, 0),
			FREQ2FBIN(5700, 0), FREQ2FBIN(5720, 0),
			FREQ2FBIN(5745, 0), FREQ2FBIN(5825, 0)
		}, {
			FREQ2FBIN(5190, 0), FREQ2FBIN(5270, 0),
			FREQ2FBIN(5310, 0), FREQ2FBIN(5510, 0),
			FREQ2FBIN(5670, 0), FREQ2FBIN(5710, 0),
			FREQ2FBIN(5755, 0), FREQ2FBIN(5795, 0)
		}, {
			FREQ2FBIN(5210, 0), FREQ2FBIN(5290, 0),
			FREQ2FBIN(5530, 0), FREQ2FBIN(5610, 0),
			FREQ2FBIN(5690, 0), FREQ2FBIN(5775, 0),
			0x00, 0x00
		}, {
			FREQ2FBIN(5180, 0), FREQ2FBIN(5260, 0),
			FREQ2FBIN(5280, 0), FREQ2FBIN(5500, 0),
			FREQ2FBIN(5700, 0), FREQ2FBIN(5720, 0),
			FREQ2FBIN(5745, 0), FREQ2FBIN(5825, 0)
		}, {
			FREQ2FBIN(5190, 0), FREQ2FBIN(5270, 0),
			FREQ2FBIN(5310, 0), FREQ2FBIN(5510, 0),
			FREQ2FBIN(5670, 0), FREQ2FBIN(5710, 0),
			FREQ2FBIN(5755, 0), FREQ2FBIN(5795, 0)
		},

		{
			FREQ2FBIN(5180, 0), FREQ2FBIN(5240, 0),
			FREQ2FBIN(5260, 0), FREQ2FBIN(5320, 0),
			FREQ2FBIN(5500, 0), FREQ2FBIN(5520, 0),
			FREQ2FBIN(5680, 0), FREQ2FBIN(5700, 0)
		}, {
			FREQ2FBIN(5180, 0), FREQ2FBIN(5240, 0),
			FREQ2FBIN(5260, 0), FREQ2FBIN(5320, 0),
			FREQ2FBIN(5500, 0), FREQ2FBIN(5520, 0),
			FREQ2FBIN(5680, 0), FREQ2FBIN(5700, 0)
		}, {
			FREQ2FBIN(5190, 0), FREQ2FBIN(5230, 0),
			FREQ2FBIN(5270, 0), FREQ2FBIN(5310, 0),
			FREQ2FBIN(5510, 0), FREQ2FBIN(5550, 0),
			FREQ2FBIN(5630, 0), FREQ2FBIN(5670, 0)
		}, {
			FREQ2FBIN(5210, 0), FREQ2FBIN(5290, 0),
			FREQ2FBIN(5530, 0), FREQ2FBIN(5610, 0),
			0x00, 0x00, 0x00, 0x00
		}, {
			FREQ2FBIN(5180, 0), FREQ2FBIN(5240, 0),
			FREQ2FBIN(5260, 0), FREQ2FBIN(5320, 0),
			FREQ2FBIN(5500, 0), FREQ2FBIN(5520, 0),
			FREQ2FBIN(5680, 0), FREQ2FBIN(5700, 0)
		}, {
			FREQ2FBIN(5190, 0), FREQ2FBIN(5230, 0),
			FREQ2FBIN(5270, 0), FREQ2FBIN(5310, 0),
			FREQ2FBIN(5510, 0), FREQ2FBIN(5550, 0),
			FREQ2FBIN(5630, 0), FREQ2FBIN(5670, 0)
		},

		{
			FREQ2FBIN(5180, 0), FREQ2FBIN(5260, 0),
			FREQ2FBIN(5280, 0), FREQ2FBIN(5500, 0),
			FREQ2FBIN(5700, 0), FREQ2FBIN(5720, 0),
			FREQ2FBIN(5745, 0), FREQ2FBIN(5825, 0)
		}, {
			FREQ2FBIN(5180, 0), FREQ2FBIN(5260, 0),
			FREQ2FBIN(5280, 0), FREQ2FBIN(5500, 0),
			FREQ2FBIN(5700, 0), FREQ2FBIN(5720, 0),
			FREQ2FBIN(5745, 0), FREQ2FBIN(5825, 0)
		}, {
			FREQ2FBIN(5190, 0), FREQ2FBIN(5270, 0),
			FREQ2FBIN(5310, 0), FREQ2FBIN(5510, 0),
			FREQ2FBIN(5670, 0), FREQ2FBIN(5710, 0),
			FREQ2FBIN(5755, 0), FREQ2FBIN(5795, 0)
		}, {
			FREQ2FBIN(5210, 0), FREQ2FBIN(5290, 0),
			FREQ2FBIN(5530, 0), FREQ2FBIN(5610, 0),
			FREQ2FBIN(5690, 0), FREQ2FBIN(5775, 0),
			0x00, 0x00
		}, {
			FREQ2FBIN(5180, 0), FREQ2FBIN(5260, 0),
			FREQ2FBIN(5280, 0), FREQ2FBIN(5500, 0),
			FREQ2FBIN(5700, 0), FREQ2FBIN(5720, 0),
			FREQ2FBIN(5745, 0), FREQ2FBIN(5825, 0)
		}, {
			FREQ2FBIN(5190, 0), FREQ2FBIN(5270, 0),
			FREQ2FBIN(5310, 0), FREQ2FBIN(5510, 0),
			FREQ2FBIN(5670, 0), FREQ2FBIN(5710, 0),
			FREQ2FBIN(5755, 0), FREQ2FBIN(5795, 0)
		}
	},
	.ctlData5G = {
		{
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 1),
			CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0),
			CTLPACK(60, 0), CTLPACK(60, 0), 0x00, 0x00
		}, {
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 1),
			CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		},

		{
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0),
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0),
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0),
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0),
			0x00, 0x00, 0x00, 0x00
		}, {
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0),
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0),
			CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0)
		},

		{
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 1),
			CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0),
			CTLPACK(60, 0), CTLPACK(60, 0), 0x00, 0x00
		}, {
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 1),
			CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 1), CTLPACK(60, 0)
		}, {
			CTLPACK(60, 1), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 1),
			CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0), CTLPACK(60, 0)
		}
	},
	.alphaThermTbl5G = {
		{
			{0x1c, 0x1c, 0x2f, 0x2b}, {0x1c, 0x1c, 0x2d, 0x2b},
			{0x1b, 0x1b, 0x2d, 0x2f}, {0x1d, 0x1d, 0x32, 0x30},
			{0x25, 0x25, 0x34, 0x3b}, {0x29, 0x29, 0x36, 0x3b},
			{0x2f, 0x2f, 0x38, 0x3a}, {0x2d, 0x2d, 0x3b, 0x3b}
		}, {
			{0x19, 0x19, 0x2a, 0x23}, {0x1b, 0x1b, 0x2a, 0x26},
			{0x19, 0x19, 0x2f, 0x2d}, {0x19, 0x19, 0x32, 0x2f},
			{0x1d, 0x1d, 0x30, 0x34}, {0x1c, 0x1c, 0x31, 0x36},
			{0x27, 0x27, 0x32, 0x3c}, {0x2a, 0x2a, 0x33, 0x3d}
		}, {
			{0x18, 0x18, 0x24, 0x40}, {0x1a, 0x1a, 0x2d, 0x42},
			{0x19, 0x19, 0x2d, 0x41}, {0x1c, 0x1c, 0x2f, 0x48},
			{0x24, 0x24, 0x30, 0x38}, {0x26, 0x26, 0x31, 0x39},
			{0x28, 0x28, 0x2d, 0x40}, {0x26, 0x26, 0x2b, 0x42}
		}
	},
};

#endif
