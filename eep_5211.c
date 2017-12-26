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

#include "atheepmgr.h"
#include "eep_5211.h"

struct eep_5211_priv {
	struct eep_5211_param {	/* Cached EEPROM parameters */
		int tgtpwr_off;	/* Target power info offset */
		int ctls_num;	/* Max number of CTLs */
	} param;
	struct ar5211_init_eep_data ini;
	struct ar5211_eeprom eep;
};

#define EEP_WORD(__off)		le16toh(aem->eep_buf[__off])

/* EEPROM as a bitstream processing context */
struct eep_bit_stream {
	int eep_off;	/* EEPROM offset, words */
	int havebits;	/* Ammount of bits in the buffer */
	uint32_t buf;	/* Buffer */
};

static uint8_t ebs_get_hi_bits(struct atheepmgr *aem,
			       struct eep_bit_stream *ebs, int bnum)
{
	uint8_t res;

	if (ebs->havebits < bnum) {
		ebs->buf = (ebs->buf << 16) | EEP_WORD(ebs->eep_off++);
		ebs->havebits += 16;
	}

	ebs->havebits -= bnum;

	res = (ebs->buf >> ebs->havebits) & ~(~0 << bnum);
	ebs->buf &= ~(~0 << ebs->havebits);

	return res;
}

#define EEP_GET_MSB(__bnum)	ebs_get_hi_bits(aem, ebs, __bnum)

static void eep_5211_fill_init_data(struct atheepmgr *aem)
{
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_init_eep_data *ini = &emp->ini;
	struct ar5211_pci_eep_data *pci = &ini->pci;
	struct ar5211_eeprom *eep = &emp->eep;
	uint16_t word;

	/* Copy data from buffer to PCI initialization structure */
	memcpy(pci, &aem->eep_buf[AR5211_EEP_PCI_DATA], sizeof(*pci));

	word = EEP_WORD(AR5211_EEP_ENDLOC_UP);
	ini->eepsz = 2 << (MS(word, AR5211_EEP_ENDLOC_SIZE) + 9);
	if (word)
		ini->eeplen = MS(word, AR5211_EEP_ENDLOC_LOC) << 16 |
			      EEP_WORD(AR5211_EEP_ENDLOC_LO);
	else
		ini->eeplen = 0;

	ini->magic = EEP_WORD(AR5211_EEP_MAGIC);
	ini->prot = EEP_WORD(AR5211_EEP_PROT);

	/* Copy customer data */
	memcpy(eep->cust_data, &aem->eep_buf[AR5211_EEP_CUST_DATA],
	       AR5211_EEP_CUST_DATA_SZ * sizeof(uint16_t));
}

static void eep_5211_fill_headers_30(struct atheepmgr *aem)
{
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	struct ar5211_base_eep_hdr *base = &eep->base;
	uint16_t word;

	emp->param.tgtpwr_off = AR5211_EEP_TGTPWR_BASE_30;

	word = EEP_WORD(AR5211_EEP_ANTGAIN_30);
	base->antgain_2g = MS(word, AR5211_EEP_ANTGAIN_2G);
	base->antgain_5g = MS(word, AR5211_EEP_ANTGAIN_5G);
}

static void eep_5211_fill_headers_33(struct atheepmgr *aem)
{
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	struct ar5211_base_eep_hdr *base = &eep->base;
	uint16_t word;

	emp->param.tgtpwr_off = AR5211_EEP_TGTPWR_BASE_33;

	word = EEP_WORD(AR5211_EEP_ANTGAIN_33);
	base->antgain_2g = MS(word, AR5211_EEP_ANTGAIN_2G);
	base->antgain_5g = MS(word, AR5211_EEP_ANTGAIN_5G);

	if (base->version >= AR5211_EEP_VER_4_0) {
		word = EEP_WORD(AR5211_EEP_MISC0);
		base->ear_off = MS(word, AR5211_EEP_EAR_OFF);
		base->xr2_dis = !!(word & AR5211_EEP_XR2_DIS);
		base->xr5_dis = !!(word & AR5211_EEP_XR5_DIS);
		base->eepmap = MS(word, AR5211_EEP_EEPMAP);

		word = EEP_WORD(AR5211_EEP_MISC1);
		base->tgtpwr_off = MS(word, AR5211_EEP_TGTPWR_OFF);
		base->exists_32khz = !!(word & AR5211_EEP_32KHZ);
		emp->param.tgtpwr_off = base->tgtpwr_off;

		word = EEP_WORD(AR5211_EEP_SRC_INFO0);
		base->ear_file_ver = MS(word, AR5211_EEP_EAR_FILE_VER);
		base->eep_file_ver = MS(word, AR5211_EEP_EEP_FILE_VER);

		word = EEP_WORD(AR5211_EEP_SRC_INFO1);
		base->ear_file_id = MS(word, AR5211_EEP_EAR_FILE_ID);
		base->art_build_num = MS(word, AR5211_EEP_ART_BUILD);
	}
	if (base->version >= AR5211_EEP_VER_5_0) {
		word = EEP_WORD(AR5211_EEP_MISC4);
		base->cal_off = MS(word, AR5211_EEP_CAL_OFF);

		word = EEP_WORD(AR5211_EEP_CAPABILITIES);
		base->comp_dis = !!(word & AR5211_EEP_COMP_DIS);
		base->aes_dis = !!(word & AR5211_EEP_AES_DIS);
		base->ff_dis = !!(word & AR5211_EEP_FF_DIS);
		base->burst_dis = !!(word & AR5211_EEP_BURST_DIS);
		base->max_qcu = MS(word, AR5211_EEP_MAX_QCU);
		base->clip_en = !(word & AR5211_EEP_CLIP_EN);

		word = EEP_WORD(AR5211_EEP_REGCAP);
		base->rd_flags = MS(word, AR5211_EEP_RD_FLAGS);
	}
}

static void eep_5211_fill_headers(struct atheepmgr *aem)
{
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	struct ar5211_base_eep_hdr *base = &eep->base;
	uint16_t word;

	word = EEP_WORD(AR5211_EEP_MAC + 0);
	base->mac[4] = (word & 0xff00) >> 8;
	base->mac[5] = (word & 0x00ff) >> 0;
	word = EEP_WORD(AR5211_EEP_MAC + 1);
	base->mac[2] = (word & 0xff00) >> 8;
	base->mac[3] = (word & 0x00ff) >> 0;
	word = EEP_WORD(AR5211_EEP_MAC + 2);
	base->mac[0] = (word & 0xff00) >> 8;
	base->mac[1] = (word & 0x00ff) >> 0;

	base->regdomain = EEP_WORD(AR5211_EEP_REGDOMAIN);
	base->checksum = EEP_WORD(AR5211_EEP_CSUM);
	base->version = EEP_WORD(AR5211_EEP_VER);

	word = EEP_WORD(AR5211_EEP_OPFLAGS);
	base->amode_en = !!(word & AR5211_EEP_AMODE);
	base->bmode_en = !!(word & AR5211_EEP_BMODE);
	base->gmode_en = !!(word & AR5211_EEP_GMODE);
	base->turbo2_dis = !!(word & AR5211_EEP_TURBO2_DIS);
	base->turbo5_maxpwr = MS(word, AR5211_EEP_TURBO5_MAXPWR);
	base->devtype = MS(word, AR5211_EEP_DEVTYPE);
	base->rfkill_en = !!(word & AR5211_EEP_RFKILL_EN);
	base->turbo5_dis = !!(word & AR5211_EEP_TURBO5_DIS);

	if (base->version >= AR5211_EEP_VER_3_3) {
		eep_5211_fill_headers_33(aem);
	} else if (base->version >= AR5211_EEP_VER_3_0) {
		eep_5211_fill_headers_30(aem);
	}
}

static void eep_5211_fill_ctl_index(struct atheepmgr *aem,
				    int off)
{
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	uint16_t word;
	int i;

	for (i = 0; i < emp->param.ctls_num; i += 2) {
		word = EEP_WORD(off + i / 2);
		eep->ctl_index[i + 0] = word >> 8;
		eep->ctl_index[i + 1] = word & 0xff;
	}
}

static void eep_5211_fill_ctl_data_30(struct atheepmgr *aem)
{
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	int off = emp->param.tgtpwr_off + AR5211_EEP_CTL_DATA;
	struct eep_bit_stream __ebs = {.eep_off = off}, *ebs = &__ebs;
	int i, j, is_2g;

	for (i = 0; i < emp->param.ctls_num; ++i) {
		ebs->havebits = 0;	/* Reset buffer contents */

		for (j = 0; j < AR5211_NUM_BAND_EDGES; ++j)
			eep->ctl_data[i][j].fbin = EEP_GET_MSB(7);

		for (j = 0; j < AR5211_NUM_BAND_EDGES; ++j)
			eep->ctl_data[i][j].pwr = EEP_GET_MSB(6);

		/* Convert edge frequency codes to modern binary format */
		is_2g = eep_ctlmodes[eep->ctl_index[i] & 0xf][0] == '2';
		for (j = 0; j < AR5211_NUM_BAND_EDGES; ++j)
			eep->ctl_data[i][j].fbin =
				FBIN_30_TO_33(eep->ctl_data[i][j].fbin, is_2g);
	}
}

static void eep_5211_fill_ctl_data_33(struct atheepmgr *aem)
{
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	int off = emp->param.tgtpwr_off + AR5211_EEP_CTL_DATA;
	uint16_t word;
	int i, j;

	for (i = 0; i < emp->param.ctls_num; ++i) {
		for (j = 0; j < AR5211_NUM_BAND_EDGES; j += 2) {
			word = EEP_WORD(off++);
			eep->ctl_data[i][j + 0].fbin = word >> 8;
			eep->ctl_data[i][j + 1].fbin = word & 0xff;
		}
		for (j = 0; j < AR5211_NUM_BAND_EDGES; j += 2) {
			word = EEP_WORD(off++);
			eep->ctl_data[i][j + 0].pwr = word >> 8;
			eep->ctl_data[i][j + 1].pwr = word & 0xff;
		}
	}
}

static bool eep_5211_fill(struct atheepmgr *aem)
{
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	struct ar5211_base_eep_hdr *base = &eep->base;
	uint16_t endloc_up, endloc_lo;
	uint16_t magic;
	int len = 0, addr;
	uint16_t *buf = aem->eep_buf;

	/* RAW magic reading with subsequent swaping requirement check */
	if (!EEP_READ(AR5211_EEP_MAGIC, &magic)) {
		fprintf(stderr, "EEPROM magic read failed\n");
		return false;
	}
	if (bswap_16(magic) == htole16(AR5211_EEPROM_MAGIC_VAL))
		aem->eep_io_swap = !aem->eep_io_swap;

	if (!EEP_READ(AR5211_EEP_ENDLOC_UP, &endloc_up) ||
	    !EEP_READ(AR5211_EEP_ENDLOC_LO, &endloc_lo)) {
		fprintf(stderr, "Unable to read EEPROM size\n");
		return false;
	}

	if (endloc_up) {
		endloc_up = le16toh(endloc_up);
		endloc_lo = le16toh(endloc_lo);
		len = ((uint32_t)MS(endloc_up, AR5211_EEP_ENDLOC_LOC) << 16) |
		      endloc_lo;
		if (len > aem->eepmap->eep_buf_sz) {
			fprintf(stderr, "EEPROM stored length is too big (%d) use maximal lenght (%zd)\n",
				len, aem->eepmap->eep_buf_sz);
			len = aem->eepmap->eep_buf_sz;
		}
	}

	if (!len) {
		printf("EEPROM length not configured, use default (%d words, %d bytes)\n",
		       AR5211_SIZE_DEF, AR5211_SIZE_DEF * 2);
		len = AR5211_SIZE_DEF;
	}

	/* Read to intermediated buffer */
	for (addr = 0; addr < len; ++addr, ++buf) {
		if (!EEP_READ(addr, buf)) {
			fprintf(stderr, "Unable to read EEPROM to buffer\n");
			return false;
		}
	}

	aem->eep_len = addr;

	memset(&emp->param, 0x00, sizeof(emp->param));

	eep_5211_fill_init_data(aem);

	eep_5211_fill_headers(aem);

	if (base->version >= AR5211_EEP_VER_3_3) {
		emp->param.ctls_num = AR5211_NUM_CTLS_33;
		eep_5211_fill_ctl_index(aem, AR5211_EEP_CTL_INDEX_33);
		eep_5211_fill_ctl_data_33(aem);
	} else if (base->version >= AR5211_EEP_VER_3_0) {
		emp->param.ctls_num = AR5211_NUM_CTLS_30;
		eep_5211_fill_ctl_index(aem, AR5211_EEP_CTL_INDEX_30);
		eep_5211_fill_ctl_data_30(aem);
	}

	return true;
}

static bool eep_5211_check(struct atheepmgr *aem)
{
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_init_eep_data *ini = &emp->ini;
	struct ar5211_eeprom *eep = &emp->eep;
	struct ar5211_base_eep_hdr *base = &eep->base;
	uint16_t sum;

	if (ini->magic != AR5211_EEPROM_MAGIC_VAL) {
		fprintf(stderr, "Invalid EEPROM Magic 0x%04x, expected 0x%04x\n",
			ini->magic, AR5211_EEPROM_MAGIC_VAL);
		return false;
	}

	if (base->version < AR5211_EEP_VER_3_0) {
		fprintf(stderr, "Bad EEPROM version 0x%04x (%d.%d)\n",
			base->version, MS(base->version, AR5211_EEP_VER_MAJ),
			MS(base->version, AR5211_EEP_VER_MIN));
		return false;
	}

	/* Checksum calculated only for "info" section (initial part should be skipped) */
	sum = eep_calc_csum((uint16_t *)aem->eep_buf + AR5211_EEP_INFO_BASE,
			    aem->eep_len - AR5211_EEP_INFO_BASE);
	if (sum != 0xffff) {
		fprintf(stderr, "Bad EEPROM checksum 0x%04x\n", sum);
		return false;
	}

	if (base->version >= AR5211_EEP_VER_4_0) {
		if (base->ear_off > aem->eep_len) {
			fprintf(stderr, "EAR data offset (0x%04x) points outside the EEPROM\n",
				base->ear_off);
			return false;
		}
		if (base->tgtpwr_off > aem->eep_len) {
			fprintf(stderr, "Target power data offset (0x%04x) points outside the EEPROM\n",
				base->tgtpwr_off);
			return false;
		}
	}
	if (base->version >= AR5211_EEP_VER_5_0) {
		if (!base->cal_off) {
			fprintf(stderr, "Invalid calibration data offset 0x%04x\n",
				base->cal_off);
			return false;
		} else if (base->cal_off > aem->eep_len) {
			fprintf(stderr, "Calibration data offset (0x%04x) points outside the EEPROM\n",
				base->cal_off);
			return false;
		}
	}

	return true;
}

static void eep_5211_dump_init_data(struct atheepmgr *aem)
{
#define PR(_token, _fmt, ...)					\
		printf("%-20s : " _fmt "\n", _token, ##__VA_ARGS__)

	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_init_eep_data *ini = &emp->ini;
	struct ar5211_pci_eep_data *pci = &ini->pci;
	int i;

	EEP_PRINT_SECT_NAME("EEPROM Init data");

	PR("Device ID", "0x%04x", le16toh(pci->dev_id));
	PR("Vendor ID", "0x%04x", le16toh(pci->ven_id));
	PR("Class code", "0x%02x", pci->class_code);
	PR("Sub class code", "0x%02x", pci->subclass_code);
	PR("Progr interface", "0x%02x", pci->prog_interface);
	PR("Revision ID", "0x%02x", pci->rev_id);
	PR("CIS ptr", "0x%08x", le16toh(pci->cis_hi) << 16 |
				le16toh(pci->cis_lo));
	PR("Ssys Device ID", "0x%04x", le16toh(pci->ssys_dev_id));
	PR("Ssys Vendor ID", "0x%04x", le16toh(pci->ssys_ven_id));
	PR("Max Lat", "0x%02x", pci->max_lat);
	PR("Min Gnt", "0x%02x", pci->min_gnt);
	PR("Int Pin", "0x%02x", pci->int_pin);
	PR("RfSilent GPIO sel", "%u", pci->rfsilent >> 2 & 0x3);
	PR("RfSilent GPIO pol", "%s", pci->rfsilent & 0x2 ? "high" : "low");

	PR("End of EAR", "0x%08x", ini->eeplen);
	PR("EEPROM size", "0x%x (%u)", ini->eepsz, ini->eepsz);
	PR("Magic", "0x%04x", ini->magic);
	for (i = 0; i < 8; ++i)
		printf("Region%d access       : %s\n", i,
		       sAccessType[(ini->prot >> (i * 2)) & 0x3]);

	printf("\n");

#undef PR
}

static void eep_5211_dump_base(struct atheepmgr *aem)
{
#define PR(_token, _fmt, ...)					\
		printf("%-20s : " _fmt "\n", _token, ##__VA_ARGS__)

	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	struct ar5211_base_eep_hdr *base = &eep->base;

	EEP_PRINT_SECT_NAME("EEPROM Base Header");

	PR("MacAddress", "%02X:%02X:%02X:%02X:%02X:%02X",
	   base->mac[0], base->mac[1], base->mac[2],
	   base->mac[3], base->mac[4], base->mac[5]);
	PR("RegDomain", "0x%04x", base->regdomain);
	if (base->version >= AR5211_EEP_VER_5_0)
		PR("RD flags", "0x%04x", base->rd_flags);
	PR("Checksum", "0x%04x", base->checksum);
	PR("Version", "%u.%u", MS(base->version, AR5211_EEP_VER_MAJ),
			       MS(base->version, AR5211_EEP_VER_MIN));
	PR("RfKill status", "%s", base->rfkill_en ? "enabled" : "disabled");
	PR("Device Type", "%s", sDeviceType[base->devtype]);
	PR("Turbo 5GHz status", "%s", base->turbo5_dis ? "disabled" : "enabled");
	if (base->version >= AR5211_EEP_VER_4_0)
		PR("Turbo 2GHz status", "%s", base->turbo2_dis ? "disabled" : "enabled");
	PR(".11a status", "%s", base->amode_en ? "enabled" : "disabled");
	PR(".11g status", "%s", base->gmode_en ? "enabled" : "disabled");
	PR(".11b status", "%s", base->bmode_en ? "enabled" : "disabled");
	if (base->version >= AR5211_EEP_VER_4_0) {
		PR("XR 5GHz status", "%s", base->xr5_dis ? "disabled" : "enabled");
		PR("XR 2GHz status", "%s", base->xr2_dis ? "disabled" : "enabled");
	}
	PR("Turbo 5G 2W pow, dBm", "%3.1f", (double)base->turbo5_maxpwr / 2);
	PR("5GHz ant gain, dBm", "%3.1f", (double)base->antgain_5g / 2);
	PR("2GHz ant gain, dBm", "%3.1f", (double)base->antgain_2g / 2);
	if (base->version >= AR5211_EEP_VER_4_0) {
		PR("EEP map", "%u", base->eepmap);
		PR("EAR offset", "0x%04x", base->ear_off);
		PR("32kHz crystal", "%s", base->exists_32khz ? "exists" : "no");
		PR("Target power offset", "0x%04x", base->tgtpwr_off);
		PR("EEP file version", "%u", base->eep_file_ver);
		PR("EAR file version", "%u", base->ear_file_ver);
		PR("EAR file id", "0x%02x", base->ear_file_id);
		PR("ART build number", "%u", base->art_build_num);
	}
	if (base->version >= AR5211_EEP_VER_5_0) {
		PR("Cal. data offset", "0x%04x", base->cal_off);
		PR("Comp status", "%s", base->comp_dis ? "disabled" : "enabled");
		PR("AES status", "%s", base->aes_dis ? "disabled" : "enabled");
		PR("FF status", "%s", base->ff_dis ? "disabled" : "enabled");
		PR("Burst status", "%s", base->burst_dis ? "disabled" : "enabled");
		PR("Max QCU", "%u", base->max_qcu);
		PR("Allow clipping", "%s", base->clip_en ? "enabled" : "disabled");
	}

	printf("\n");

#undef PR
}

static void eep_5211_dump_ctl_edges(const struct ar5211_ctl_edge *edges,
				    int is_2g)
{
	int i, open;

	printf("           Edges, MHz:");
	for (i = 0, open = 1; i < AR5211_NUM_BAND_EDGES && edges[i].fbin; ++i) {
		printf(" %c%4u%c",
		       !CTL_EDGE_FLAGS(edges[i].pwr) && open ? '[' : ' ',
		       FBIN2FREQ(edges[i].fbin, is_2g),
		       !CTL_EDGE_FLAGS(edges[i].pwr) && !open ? ']' : ' ');
		if (!CTL_EDGE_FLAGS(edges[i].pwr))
			open = !open;
	}
	printf("\n");
	printf("      MaxTxPower, dBm:");
	for (i = 0; i < AR5211_NUM_BAND_EDGES && edges[i].fbin; ++i)
		printf("  %4.1f ", (double)CTL_EDGE_POWER(edges[i].pwr) / 2);
	printf("\n");
}

static void eep_5211_dump_ctl(const uint8_t *index,
			      const struct ar5211_ctl_edge *data,
			      int maxctl)
{
	int i;
	uint8_t ctl;

	for (i = 0; i < maxctl; ++i) {
		if (!index[i])
			break;
		ctl = index[i];
		printf("    %s %s:\n", eep_ctldomains[ctl >> 4],
		       eep_ctlmodes[ctl & 0xf]);

		eep_5211_dump_ctl_edges(data + i * AR5211_NUM_BAND_EDGES,
					eep_ctlmodes[ctl & 0xf][0]=='2'/*:)*/);

		printf("\n");
	}
}

static void eep_5211_dump_power(struct atheepmgr *aem)
{
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;

	EEP_PRINT_SECT_NAME("EEPROM Power Info");

	EEP_PRINT_SUBSECT_NAME("CTL data");
	eep_5211_dump_ctl(eep->ctl_index, &eep->ctl_data[0][0],
			  emp->param.ctls_num);
}

static bool eep_5211_update_eeprom(struct atheepmgr *aem, int param,
				   const void *data)
{
#ifdef CONFIG_I_KNOW_WHAT_I_AM_DOING
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	struct ar5211_base_eep_hdr *base = &eep->base;
#endif
	uint16_t *buf = aem->eep_buf;
	int data_pos, data_len = 0, addr, el, i;
	uint16_t sum;

	switch (param) {
	case EEP_UPDATE_MAC:
		data_pos = AR5211_EEP_MAC;
		data_len = 6 / sizeof(uint16_t);
		for (i = 0; i < 6; ++i) {
			((uint8_t *)(buf + AR5211_EEP_MAC))[5 - i] =
							((uint8_t *)data)[i];
		}
		break;
#ifdef CONFIG_I_KNOW_WHAT_I_AM_DOING
	case EEP_ERASE_CTL:
		/* It is enough to erase the CTL index only */
		data_pos = base->version >= AR5211_EEP_VER_3_3 ?
			   AR5211_EEP_CTL_INDEX_33 : AR5211_EEP_CTL_INDEX_30;
		data_len = emp->param.ctls_num / 2;
		for (addr = data_pos; addr < (data_pos + data_len); ++addr)
			buf[addr] = 0x0000;
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
	if (data_pos > AR5211_EEP_INFO_BASE) {
		el = aem->eep_len - AR5211_EEP_INFO_BASE;
		buf[AR5211_EEP_CSUM] = 0xffff;
		sum = eep_calc_csum(&buf[AR5211_EEP_INFO_BASE], el);
		buf[AR5211_EEP_CSUM] = sum;
		if (!EEP_WRITE(AR5211_EEP_CSUM, sum)) {
			fprintf(stderr, "Unable to update EEPROM checksum\n");
			return false;
		}
	}

	return true;
}

const struct eepmap eepmap_5211 = {
	.name = "5211",
	.desc = "Legacy .11abg chips EEPROM map (AR5211/AR5212/AR5414/etc.)",
	.priv_data_sz = sizeof(struct eep_5211_priv),
	.eep_buf_sz = AR5211_SIZE_MAX,
	.fill_eeprom = eep_5211_fill,
	.check_eeprom = eep_5211_check,
	.dump = {
		[EEP_SECT_INIT] = eep_5211_dump_init_data,
		[EEP_SECT_BASE] = eep_5211_dump_base,
		[EEP_SECT_POWER] = eep_5211_dump_power,
	},
	.update_eeprom = eep_5211_update_eeprom,
	.params_mask = BIT(EEP_UPDATE_MAC)
#ifdef CONFIG_I_KNOW_WHAT_I_AM_DOING
		| BIT(EEP_ERASE_CTL)
#endif
	,
};
