/*
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

#ifndef EEP_5211_H
#define EEP_5211_H

/* Frequency pack/unpack macros for EEPROM version prior to 3.3 */
#define FREQ2FBIN_30_2G(f)	((f) - 2400)
#define FREQ2FBIN_30_5G(f)	((f) <= 5720 ? ((f)-5100)/10 : ((f)-5720)/5+62)
#define FREQ2FBIN_30(f, is_2g)	(is_2g ? FREQ2FBIN_30_2G(f):FREQ2FBIN_30_5G(f))
#define FBIN2FREQ_30_2G(b)	((b) + 2400)
#define FBIN2FREQ_30_5G(b)	((b) <= 62 ? 5100+10*(b) : 5720+5*((b)-62))
#define FBIN2FREQ_30(b, is_2g)	(is_2g ? FBIN2FREQ_30_2G(b):FBIN2FREQ_30_5G(b))

/* Re-encoding fbin of v3.0 to fbin of v3.3 via frequency */
#define FBIN_30_TO_33(b, is_2g)				\
		(!(b) ? 0 : FREQ2FBIN(FBIN2FREQ_30(b, is_2g), is_2g))

#define AR5211_EEPROM_MAGIC_VAL		0x5aa5

/* This max value is almost arbitrary and it selection is based on my
 * own observations. */
#define AR5211_SIZE_MAX			0x2000		/* 16KB EEPROM */
#define AR5211_SIZE_DEF			0x0400		/* 2KB EEPROM */

#define AR5211_EEP_VER_3_0		0x3000
#define AR5211_EEP_VER_3_3		0x3003
#define AR5211_EEP_VER_4_0		0x4000
#define AR5211_EEP_VER_5_0		0x5000

#define AR5211_NUM_CTLS_30		16
#define AR5211_NUM_CTLS_33		32
#define AR5211_NUM_CTLS_MAX		AR5211_NUM_CTLS_33
#define AR5211_NUM_BAND_EDGES		8

/**
 * NB: the EEPROM layout for legacy chips is not byte-aligned, so there are
 * quite impossible to represent it as a C structure. So we should use here
 * the same approach as the drivers used: we define a structure that keeps
 * only the EERPOM data and fill it during the initialization procedure by
 * processing EEPROM contents in a word-by-word manner. Also, define a set of
 * macros to facilitate the EEPROM parsing and keep the code a magic-less.
 */

#define AR5211_EEP_PCI_DATA		0x0000

#define AR5211_EEP_ENDLOC_LO		0x001b		/* Lower word */

#define AR5211_EEP_ENDLOC_UP		0x001c		/* Upper word */
#define AR5211_EEP_ENDLOC_SIZE		0x000f		/* EEPROM size */
#define AR5211_EEP_ENDLOC_SIZE_S	0
#define AR5211_EEP_ENDLOC_LOC		0xfff0		/* End location */
#define AR5211_EEP_ENDLOC_LOC_S		4

#define AR5211_EEP_MAC			0x001d

#define AR5211_EEP_MAGIC		0x003d

#define AR5211_EEP_PROT			0x003f

#define AR5211_EEP_CUST_DATA		0x00b0
#define AR5211_EEP_CUST_DATA_SZ		0x0f		/* Region size, words */

#define AR5211_EEP_REGDOMAIN		0x00bf

#define AR5211_EEP_INFO_BASE		0x00c0

#define AR5211_EEP_CSUM			(AR5211_EEP_INFO_BASE + 0x00)

#define AR5211_EEP_VER			(AR5211_EEP_INFO_BASE + 0x01)
#define AR5211_EEP_VER_MAJ		0xf000
#define AR5211_EEP_VER_MAJ_S		12
#define AR5211_EEP_VER_MIN		0x0fff
#define AR5211_EEP_VER_MIN_S		0

#define AR5211_EEP_OPFLAGS		(AR5211_EEP_INFO_BASE + 0x02)
#define AR5211_EEP_MODES		0x0007
#define AR5211_EEP_MODES_S		0
#define AR5211_EEP_AMODE		BIT(0)
#define AR5211_EEP_BMODE		BIT(1)
#define AR5211_EEP_GMODE		BIT(2)
#define AR5211_EEP_TURBO2_DIS		BIT(3)	/* 2GHz turbo disabled */
#define AR5211_EEP_TURBO5_MAXPWR	0x07f0
#define AR5211_EEP_TURBO5_MAXPWR_S	4
#define AR5211_EEP_DEVTYPE		0x3800
#define AR5211_EEP_DEVTYPE_S		11
#define AR5211_EEP_RFKILL_EN		BIT(14)
#define AR5211_EEP_TURBO5_DIS		BIT(15)

#define AR5211_EEP_ANTGAIN_30		(AR5211_EEP_INFO_BASE + 0x04)
#define AR5211_EEP_ANTGAIN_33		(AR5211_EEP_INFO_BASE + 0x03)
#define AR5211_EEP_ANTGAIN_2G		0x00ff
#define AR5211_EEP_ANTGAIN_2G_S		0
#define AR5211_EEP_ANTGAIN_5G		0xff00
#define AR5211_EEP_ANTGAIN_5G_S		8

#define AR5211_EEP_MISC0		(AR5211_EEP_INFO_BASE + 0x04)
#define AR5211_EEP_EAR_OFF		0x0fff
#define AR5211_EEP_EAR_OFF_S		0
#define AR5211_EEP_XR2_DIS		BIT(12)
#define AR5211_EEP_XR5_DIS		BIT(13)
#define AR5211_EEP_EEPMAP		0xc000
#define AR5211_EEP_EEPMAP_S		14

#define AR5211_EEP_MISC1		(AR5211_EEP_INFO_BASE + 0x05)
#define AR5211_EEP_TGTPWR_OFF		0x0fff
#define AR5211_EEP_TGTPWR_OFF_S		0
#define AR5211_EEP_32KHZ		BIT(14)

/* aka MISC2 in ath5k */
#define AR5211_EEP_SRC_INFO0		(AR5211_EEP_INFO_BASE + 0x06)
#define AR5211_EEP_EAR_FILE_VER		0x00ff
#define AR5211_EEP_EAR_FILE_VER_S	0
#define AR5211_EEP_EEP_FILE_VER		0xff00
#define AR5211_EEP_EEP_FILE_VER_S	8

/* aka MISC3 in ath5k */
#define AR5211_EEP_SRC_INFO1		(AR5211_EEP_INFO_BASE + 0x07)
#define AR5211_EEP_EAR_FILE_ID		0x00ff
#define AR5211_EEP_EAR_FILE_ID_S	0
#define AR5211_EEP_ART_BUILD		0xfc00
#define AR5211_EEP_ART_BUILD_S		10

#define AR5211_EEP_MISC4		(AR5211_EEP_INFO_BASE + 0x08)
#define AR5211_EEP_CAL_OFF		0xfff0
#define AR5211_EEP_CAL_OFF_S		4

/* aka MISC5 in ath5k */
#define AR5211_EEP_CAPABILITIES		(AR5211_EEP_INFO_BASE + 0x09)
#define AR5211_EEP_COMP_DIS		BIT(0)
#define AR5211_EEP_AES_DIS		BIT(1)
#define AR5211_EEP_FF_DIS		BIT(2)
#define AR5211_EEP_BURST_DIS		BIT(3)
#define AR5211_EEP_MAX_QCU		0x01f0
#define AR5211_EEP_MAX_QCU_S		4
#define AR5211_EEP_CLIP_EN		BIT(9)

/* aka MISC5 in ath5k */
#define AR5211_EEP_REGCAP		(AR5211_EEP_INFO_BASE + 0x0a)
#define AR5211_EEP_RD_FLAGS		0xffc0
#define AR5211_EEP_RD_FLAGS_S		6

#define AR5211_EEP_CTL_INDEX_30		(AR5211_EEP_INFO_BASE + 0x24)
#define AR5211_EEP_CTL_INDEX_33		(AR5211_EEP_INFO_BASE + 0x68)

#define AR5211_EEP_TGTPWR_BASE_30	(AR5211_EEP_INFO_BASE + 0x95)
#define AR5211_EEP_TGTPWR_BASE_33	(AR5211_EEP_INFO_BASE + 0xe5)
#define AR5211_EEP_CTL_DATA		0x001a

struct ar5211_pci_eep_data {
	uint16_t dev_id;
	uint16_t ven_id;
	uint8_t subclass_code;
	uint8_t class_code;
	uint8_t rev_id;
	uint8_t prog_interface;
	uint16_t reserved1;
	uint16_t cis_hi;
	uint16_t cis_lo;
	uint16_t ssys_dev_id;
	uint16_t ssys_ven_id;
	uint8_t min_gnt;
	uint8_t max_lat;
	uint8_t zeroes1;
	uint8_t int_pin;
	uint16_t reserved2;
	uint16_t pm_cap;
	uint8_t zeroes2;
	uint8_t pm_data_scale;
	uint8_t pm_data_d0;
	uint8_t pm_data_d3;
	uint16_t rfsilent;	/* + CLKRUN_EN flag */
	uint16_t reserved3[11];
} __attribute__((packed));

struct ar5211_init_eep_data {
	struct ar5211_pci_eep_data pci;
	uint32_t eepsz;
	uint32_t eeplen;
	uint16_t magic;
	uint16_t prot;
};

struct ar5211_base_eep_hdr {
	uint8_t mac[6];
	uint16_t regdomain;
	uint16_t rd_flags;
	uint16_t checksum;
	uint16_t version;
	int amode_en:1;
	int bmode_en:1;
	int gmode_en:1;
	int turbo2_dis:1;
	int turbo5_dis:1;
	int rfkill_en:1;
	int xr2_dis:1;
	int xr5_dis:1;
	int exists_32khz:1;
	int comp_dis:1;
	int aes_dis:1;
	int ff_dis:1;
	int burst_dis:1;
	int clip_en:1;
	uint8_t turbo5_maxpwr;
	uint8_t devtype;
	uint8_t antgain_2g;
	uint8_t antgain_5g;
	uint16_t ear_off;
	uint8_t eepmap;
	uint16_t tgtpwr_off;
	uint8_t eep_file_ver;
	uint8_t ear_file_ver;
	uint8_t ear_file_id;
	uint8_t art_build_num;
	uint16_t cal_off;
	uint8_t max_qcu;
};

struct ar5211_ctl_edge {
	uint8_t fbin;
	uint8_t pwr;
};

struct ar5211_eeprom {
	struct ar5211_base_eep_hdr base;
	uint8_t cust_data[AR5211_EEP_CUST_DATA_SZ * 2];
	uint8_t ctl_index[AR5211_NUM_CTLS_MAX];
	struct ar5211_ctl_edge ctl_data[AR5211_NUM_CTLS_MAX][AR5211_NUM_BAND_EDGES];
};

#endif	/* EEP_5211_H */
