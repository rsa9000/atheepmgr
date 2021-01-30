/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * Copyright (c) 2013,2020 Sergey Ryazanov <ryazanov.s.a@gmail.com>
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

#ifndef EEP_COMMON_H
#define EEP_COMMON_H

/**
 * NB: there are no portable way to force endian for static values, so create
 * the custom one.
 */
#if __BYTE_ORDER == __BIG_ENDIAN
#define LE16CONST(__val)	((((uint16_t)(__val) & 0x00ff) << 8) | \
				 (((uint16_t)(__val) & 0xff00) >> 8))
#define LE32CONST(__val)	((((uint32_t)(__val) & 0x000000ff) << 24) | \
				 (((uint32_t)(__val) & 0x0000ff00) <<  8) | \
				 (((uint32_t)(__val) & 0x00ff0000) >>  8) | \
				 (((uint32_t)(__val) & 0xff000000) >> 24))
#else
#define LE16CONST(__val)	((uint16_t)(__val))
#define LE32CONST(__val)	((uint32_t)(__val))
#endif

#if __BYTE_ORDER == __BIG_ENDIAN
#define AR5416_EEPROM_MAGIC	0x5aa5
#else
#define AR5416_EEPROM_MAGIC	0xa55a
#endif

#define AR5416_EEPROM_MAGIC_OFFSET	0x0
#define AR5416_EEP_NO_BACK_VER		0x1
#define AR5416_EEP_VER			0xE

#define AR5416_OPFLAGS_11A		0x01
#define AR5416_OPFLAGS_11G		0x02
#define AR5416_OPFLAGS_N_5G_HT40	0x04
#define AR5416_OPFLAGS_N_2G_HT40	0x08
#define AR5416_OPFLAGS_N_5G_HT20	0x10
#define AR5416_OPFLAGS_N_2G_HT20	0x20

#define AR5416_RFSILENT_ENABLED			0x0001
#define AR5416_RFSILENT_POLARITY		0x0002
#define AR5416_RFSILENT_POLARITY_S		1
#define AR5416_RFSILENT_GPIO_SEL		0x001c
#define AR5416_RFSILENT_GPIO_SEL_S		2

#define AR5416_EEPMISC_BIG_ENDIAN		0x01

#define AR5416_EEP_MINOR_VER_3			0x3
#define AR5416_EEP_MINOR_VER_17			0x11
#define AR5416_EEP_MINOR_VER_19			0x13
#define AR5416_EEP_MINOR_VER_21			0x15

#define AR5416_ANTCTRLCHAIN_IDLE		0x0003
#define AR5416_ANTCTRLCHAIN_IDLE_S		0
#define AR5416_ANTCTRLCHAIN_TX			0x000c
#define AR5416_ANTCTRLCHAIN_TX_S		2
#define AR5416_ANTCTRLCHAIN_RX_NOATT		0x0030
#define AR5416_ANTCTRLCHAIN_RX_NOATT_S		4
#define AR5416_ANTCTRLCHAIN_RX_ATT1		0x00c0
#define AR5416_ANTCTRLCHAIN_RX_ATT1_S		6
#define AR5416_ANTCTRLCHAIN_RX_ATT12		0x0300
#define AR5416_ANTCTRLCHAIN_RX_ATT12_S		8
#define AR5416_ANTCTRLCHAIN_BT			0x0c00
#define AR5416_ANTCTRLCHAIN_BT_S		10

#define AR5416_ANTCTRLCMN_IDLE			0x000f
#define AR5416_ANTCTRLCMN_IDLE_S		0
#define AR5416_ANTCTRLCMN_TX			0x00f0
#define AR5416_ANTCTRLCMN_TX_S			4
#define AR5416_ANTCTRLCMN_RX			0x0f00
#define AR5416_ANTCTRLCMN_RX_S			8
#define AR5416_ANTCTRLCMN_BT			0xf000
#define AR5416_ANTCTRLCMN_BT_S			12

#define AR5416_EEPROM_MODAL_SPURS		5
#define AR5416_NUM_PD_GAINS			4
#define AR5416_PD_GAIN_ICEPTS			5
#define AR5416_BCHAN_UNUSED			0xff

#define AR5416_PWR_TABLE_OFFSET_DB		-5

#define AR5416_NUM_TARGET_POWER_RATES_LEG	4
#define AR5416_NUM_TARGET_POWER_RATES_HT	8

#define PWR2X(__dbm)				((unsigned int)(__dbm * 2))

#define CTL_EDGE_POWER(__ctl)			((__ctl) & 0x3f)
#define CTL_EDGE_FLAGS(__ctl)			(((__ctl) & 0xc0) >> 6)
#define CTLPACK(_tpower, _flag)			((_tpower) | ((_flag) << 6))

#define FREQ2FBIN(f, is_2g)			\
		((uint8_t)((is_2g) ? (f) - 2300 : ((f) - 4800) / 5))
#define FBIN2FREQ(b, is_2g)	((is_2g) ? (b) + 2300 : (b) * 5 + 4800)

#define AR9300_COMP_HDR_LEN		4
#define AR9300_COMP_CKSUM_LEN		2

enum ar9300_compression_types {
	AR9300_COMP_NONE = 0,
	AR9300_COMP_LZMA,
	AR9300_COMP_PAIRS,
	AR9300_COMP_BLOCK,
};

/* Unpacked EEPROM compression header */
struct ar9300_comp_hdr {
	int comp;	/* Compression type */
	int ref;	/* Reference EEPROM data */
	int len;	/* Data length */
	int maj;
	int min;
};

extern const char * const sDeviceType[];
extern const char * const sAccessType[];
extern const char * const eep_rates_cck[AR5416_NUM_TARGET_POWER_RATES_LEG];
extern const char * const eep_rates_ofdm[AR5416_NUM_TARGET_POWER_RATES_LEG];
extern const char * const eep_rates_ht[AR5416_NUM_TARGET_POWER_RATES_HT];
extern const char * const eep_ctldomains[];
extern const char * const eep_ctlmodes[];

struct ar5416_eep_reg_init {
	uint16_t addr;
	uint16_t val_low;
	uint16_t val_high;
} __attribute__ ((packed));

struct ar5416_eep_init {
	uint16_t magic;
	uint16_t prot;
	uint16_t iptr;
	struct ar5416_eep_reg_init regs[];
} __attribute__ ((packed));

struct ar5416_spur_chan {
	uint16_t spurChan;
	uint8_t spurRangeLow;
	uint8_t spurRangeHigh;
} __attribute__ ((packed));

struct ar5416_cal_ctl_edges {
	uint8_t bChannel;
	uint8_t ctl;
} __attribute__ ((packed));

struct ar5416_cal_target_power_leg {
	uint8_t bChannel;
	uint8_t tPow2x[AR5416_NUM_TARGET_POWER_RATES_LEG];
} __attribute__ ((packed));

struct ar5416_cal_target_power_ht {
	uint8_t bChannel;
	uint8_t tPow2x[AR5416_NUM_TARGET_POWER_RATES_HT];
} __attribute__ ((packed));

struct ar5416_cal_target_power {
	uint8_t bChannel;
	uint8_t tPow2x[];
} __attribute__ ((packed));

#define EEP_PRINT_SECT_NAME(__name)			\
		printf("\n.----------------------.\n");	\
		printf("| %-20s |\n", __name);		\
		printf("'----------------------'\n\n");
#define EEP_PRINT_SUBSECT_NAME(__name)			\
		printf("[%s]\n\n", __name);

/**
 * All EEPROM maps in scope have a similar fields structure and names, but
 * different offsets within a base header and (or) different start position of
 * the main data within EEPROM. So use macro to overcome offset differences.
 */
bool __ar5416_toggle_byteswap(struct atheepmgr *aem, uint32_t eepmisc_off,
			      uint32_t binbuildnum_off);
#define AR5416_TOGGLE_BYTESWAP(__chip)					\
	__ar5416_toggle_byteswap(aem,					\
				 AR ## __chip ## _DATA_START_LOC +	\
				 offsetof(struct ar ## __chip ## _eeprom,\
				          baseEepHeader.eepMisc) / 2,	\
				 AR ## __chip ## _DATA_START_LOC +	\
				 offsetof(struct ar ## __chip ## _eeprom,\
				          baseEepHeader.binBuildNumber) / 2)

void ar5416_dump_eep_init(const struct ar5416_eep_init *ini, size_t size);

void ar5416_dump_target_power(const struct ar5416_cal_target_power *pow,
			      int maxchans, const char * const rates[],
			      int nrates, int is_2g);
void ar5416_dump_ctl(const uint8_t *index,
		     const struct ar5416_cal_ctl_edges *data,
		     int maxctl, int maxchains, int maxradios, int maxedges);

#define EEP_FIELD_OFFSET(__field)					\
		(offsetof(typeof(*eep), __field) / sizeof(uint16_t))
#define EEP_FIELD_SIZE(__field)						\
		(sizeof(eep->__field) / sizeof(uint16_t))

void ar9300_comp_hdr_unpack(const uint8_t *p, struct ar9300_comp_hdr *hdr);
uint16_t ar9300_comp_cksum(const uint8_t *data, int dsize);
int ar9300_compress_decision(struct atheepmgr *aem, int it,
			     struct ar9300_comp_hdr *hdr, uint8_t *out,
			     const uint8_t *data, int out_size, int *pcurrref,
			     const uint8_t *(*tpl_lookup_cb)(int));

uint16_t eep_calc_csum(const uint16_t *buf, size_t len);

#endif /* EEP_COMMON_H */
