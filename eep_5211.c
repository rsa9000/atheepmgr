/*
 * Copyright (c) 2013,2016-2020 Sergey Ryazanov <ryazanov.s.a@gmail.com>
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
#include "eep_5211.h"

struct eep_5211_priv {
	struct eep_5211_param {	/* Cached EEPROM parameters */
		int eepmap;	/* EEPROM map type */
		int pdcal_off;	/* PD calibration info offset */
		int tgtpwr_off;	/* Target power info offset */
		struct eep_5211_pdcal_param {
			const uint8_t *piers;
			int npiers;
			int8_t gains[AR5211_MAX_PDCAL_GAINS];/* dB */
			int ngains;
			int nicepts[AR5211_MAX_PDCAL_GAINS];
		} pdcal_a, pdcal_b, pdcal_g;	/* PD calibration params */
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

/**
 * EEPROM parameter fields can occupy an arbitrary number of bits. To simplify
 * the code, we use a function, which reads EEPROM in a word-by-word manner,
 * stores data which was read in the buffer, and returns an arbitrary number
 * of bits in each call.
 *
 * Some EEPROM fields begin in one word, cross the word boundary and continue
 * in the next one. There are two ways to store bits of such fields:
 * 1. the LSB(s) of the field are stored in the MSB(s) of the current word, and
 *    the MSB(s) of the field are stored in the LSB(s) of the next word of
 *    EEPROM;
 * 2. the MSB(s) of the field are stored in the LSB(s) of the current word, and
 *    the LSB(s) of the field are stored in the MSB(s) of the next word of
 *    EEPROM.
 *
 * Since both approaches are actively used, we need two types of the bits
 * fetching functions:
 * 1. function of the first type appends the next read EEPROM word to the buffer
 *    to the right (places the new word as LSB of the buffer) and returns
 *    requested number of MSBs of the buffer;
 * 2. function of the second type appends the next read EEPROM word to the buffer
 *    to the left (places the new word as MSB of the buffer) and returns
 *    requested number of MSBs of the buffer.
 */

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

static uint8_t ebs_get_lo_bits(struct atheepmgr *aem,
			       struct eep_bit_stream *ebs, int bnum)
{
	uint8_t res;

	if (ebs->havebits < bnum) {
		ebs->buf |= EEP_WORD(ebs->eep_off++) << ebs->havebits;
		ebs->havebits += 16;
	}

	res = ebs->buf & ~(~0 << bnum);
	ebs->buf >>= bnum;
	ebs->havebits -= bnum;

	return res;
}

#define EEP_GET_MSB(__bnum)	ebs_get_hi_bits(aem, ebs, __bnum)
#define EEP_GET_LSB(__bnum)	ebs_get_lo_bits(aem, ebs, __bnum)

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

	emp->param.pdcal_off = AR5211_EEP_PDCAL_BASE_30;
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

	emp->param.pdcal_off = AR5211_EEP_PDCAL_BASE_33;
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
		emp->param.eepmap = base->eepmap;

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
		emp->param.pdcal_off = base->cal_off;

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

static void eep_5211_parse_modal_cmn1(struct atheepmgr *aem,
				      struct eep_bit_stream *ebs,
				      struct ar5211_modal_eep_hdr *modal)
{
	int i;

	EEP_GET_MSB(1);		/* Take away unused bit */
	modal->sw_settle_time = EEP_GET_MSB(7);
	modal->txrx_atten = EEP_GET_MSB(6);

	for (i = 0; i < ARRAY_SIZE(modal->ant_ctrl); ++i)
		modal->ant_ctrl[i] = EEP_GET_MSB(6);

	modal->adc_desired_size = EEP_GET_MSB(8);
}

static void eep_5211_parse_modal_cmn2(struct atheepmgr *aem,
				      struct eep_bit_stream *ebs,
				      struct ar5211_modal_eep_hdr *modal)
{
	modal->tx_end_to_xlna_on = EEP_GET_MSB(8);
	modal->thresh62 = EEP_GET_MSB(8);
	modal->tx_end_to_xpa_off = EEP_GET_MSB(8);
	modal->tx_frame_to_xpa_on = EEP_GET_MSB(8);
	modal->pga_desired_size = EEP_GET_MSB(8);
	modal->nfthresh = EEP_GET_MSB(8);
	EEP_GET_MSB(2);		/* Skip unused bits */
	modal->fixed_bias = EEP_GET_MSB(1);	/* A & G only */
	modal->xlna_gain = EEP_GET_MSB(8);
	modal->xpd_gain = EEP_GET_MSB(4);
	modal->xpd = EEP_GET_MSB(1);
}

static void eep_5211_parse_modal_a(struct atheepmgr *aem, int off)
{
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	struct ar5211_modal_eep_hdr *modal = &eep->modal_a;
	struct eep_bit_stream __ebs = {.eep_off = off, .havebits = 0};
	struct eep_bit_stream *ebs = &__ebs;
	int i;

	eep_5211_parse_modal_cmn1(aem, ebs, modal);

	for (i = 0; i < 4; ++i) {
		modal->pa_ob[3 - i] = EEP_GET_MSB(3);
		modal->pa_db[3 - i] = EEP_GET_MSB(3);
	}

	eep_5211_parse_modal_cmn2(aem, ebs, modal);

	if (eep->base.version < AR5211_EEP_VER_3_3)
		return;

	/* Here we change the bits fetching direction */

	modal->xr_tgt_pwr = EEP_GET_LSB(6);
	modal->false_detect_backoff = EEP_GET_LSB(7);

	if (eep->base.version < AR5211_EEP_VER_3_4)
		return;

	modal->pd_gain_init = EEP_GET_LSB(6);

	if (eep->base.version < AR5211_EEP_VER_4_0)
		return;

	modal->iq_cal_q = EEP_GET_LSB(5);
	modal->iq_cal_i = EEP_GET_LSB(6);
	EEP_GET_LSB(2);		/* Skip unused bits */

	if (eep->base.version < AR5211_EEP_VER_4_1)
		return;

	modal->rxtx_margin = EEP_GET_LSB(6);

	if (eep->base.version < AR5211_EEP_VER_5_0)
		return;

	modal->turbo_sw_settle_time = EEP_GET_LSB(7);
	modal->turbo_txrx_atten = EEP_GET_LSB(6);
	modal->turbo_rxtx_margin = EEP_GET_LSB(6);
	modal->turbo_adc_desired_size = EEP_GET_LSB(8);
	modal->turbo_pga_desired_size = EEP_GET_LSB(8);
}

static void eep_5211_parse_modal_b(struct atheepmgr *aem, int off)
{
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	struct ar5211_modal_eep_hdr *modal = &eep->modal_b;
	struct eep_bit_stream __ebs = {.eep_off = off, .havebits = 0};
	struct eep_bit_stream *ebs = &__ebs;

	eep_5211_parse_modal_cmn1(aem, ebs, modal);

	modal->pa_ob[0] = EEP_GET_MSB(4) & 0x07;
	modal->pa_db[0] = EEP_GET_MSB(4) & 0x07;

	eep_5211_parse_modal_cmn2(aem, ebs, modal);

	if (eep->base.version < AR5211_EEP_VER_3_3)
		return;

	/* Here we change the bits fetching direction */

	modal->pa_ob_2ghz = EEP_GET_LSB(3);
	modal->pa_db_2ghz = EEP_GET_LSB(3);
	modal->false_detect_backoff = EEP_GET_LSB(7);

	if (eep->base.version < AR5211_EEP_VER_3_4)
		return;

	modal->pd_gain_init = EEP_GET_LSB(6);
	EEP_GET_LSB(13);	/* Skip unused bits */

	if (eep->base.version < AR5211_EEP_VER_4_0)
		return;

	modal->cal_piers[0] = EEP_GET_LSB(8);
	modal->cal_piers[1] = EEP_GET_LSB(8);
	modal->cal_piers[2] = EEP_GET_LSB(8);

	if (eep->base.version < AR5211_EEP_VER_4_1)
		return;

	modal->rxtx_margin = EEP_GET_LSB(6);
}

static void eep_5211_parse_modal_g(struct atheepmgr *aem, int off)
{
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	struct ar5211_modal_eep_hdr *modal = &eep->modal_g;
	struct eep_bit_stream __ebs = {.eep_off = off, .havebits = 0};
	struct eep_bit_stream *ebs = &__ebs;
	uint8_t ch14_filter_cck_delta, rxtx_margin;

	eep_5211_parse_modal_cmn1(aem, ebs, modal);

	modal->pa_ob[0] = EEP_GET_MSB(4) & 0x07;
	modal->pa_db[0] = EEP_GET_MSB(4) & 0x07;

	eep_5211_parse_modal_cmn2(aem, ebs, modal);

	if (eep->base.version < AR5211_EEP_VER_3_3)
		return;

	/* Here we change the bits fetching direction */

	modal->pa_ob_2ghz = EEP_GET_LSB(3);
	modal->pa_db_2ghz = EEP_GET_LSB(3);
	modal->false_detect_backoff = EEP_GET_LSB(7);

	if (eep->base.version < AR5211_EEP_VER_3_4)
		return;

	modal->pd_gain_init = EEP_GET_LSB(6);
	modal->cck_ofdm_pwr_delta = EEP_GET_LSB(8);

	if (eep->base.version < AR5211_EEP_VER_4_0)
		return;

	ch14_filter_cck_delta = EEP_GET_LSB(5);

	modal->cal_piers[0] = EEP_GET_LSB(8);
	modal->cal_piers[1] = EEP_GET_LSB(8);

	modal->turbo_maxtxpwr_2w = EEP_GET_LSB(7);
	modal->xr_tgt_pwr = EEP_GET_LSB(6);
	EEP_GET_LSB(3);			/* Skip unused bits */

	modal->cal_piers[2] = EEP_GET_LSB(8);
	rxtx_margin = EEP_GET_LSB(6);	/* Preserve bits for a while */
	EEP_GET_LSB(2);			/* Skip unused bits */

	modal->iq_cal_q = EEP_GET_LSB(5);
	modal->iq_cal_i = EEP_GET_LSB(6);
	EEP_GET_LSB(5);			/* Skip unused bits */

	if (eep->base.version < AR5211_EEP_VER_4_1)
		return;

	modal->rxtx_margin = rxtx_margin;

	if (eep->base.version < AR5211_EEP_VER_4_2)
		return;

	modal->cck_ofdm_gain_delta = EEP_GET_LSB(8);

	if (eep->base.version < AR5211_EEP_VER_4_6)
		return;

	modal->ch14_filter_cck_delta = ch14_filter_cck_delta;

	if (eep->base.version < AR5211_EEP_VER_5_0)
		return;

	modal->turbo_sw_settle_time = EEP_GET_LSB(7);
	modal->turbo_txrx_atten = EEP_GET_LSB(6);
	modal->turbo_rxtx_margin = EEP_GET_LSB(6);
	modal->turbo_adc_desired_size = EEP_GET_LSB(8);
	modal->turbo_pga_desired_size = EEP_GET_LSB(8);
}

/**
 * This data is stored after the CTL index information and it acomplishes data
 * fetched from the B & G modal headers.
 */
static void eep_5211_parse_modal_ext_31(struct atheepmgr *aem)
{
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	struct ar5211_modal_eep_hdr *modal_b = &eep->modal_b;
	struct ar5211_modal_eep_hdr *modal_g = &eep->modal_g;
	int off = AR5211_EEP_MODAL_EXT_31;
	uint16_t word;

	word = EEP_WORD(off++);
	modal_b->pa_ob_2ghz = (word & 0x07) >> 0;
	modal_b->pa_db_2ghz = (word & 0x38) >> 3;

	word = EEP_WORD(off++);
	modal_g->pa_ob_2ghz = (word & 0x07) >> 0;
	modal_g->pa_db_2ghz = (word & 0x38) >> 3;
}

static void eep_5211_fill_headers(struct atheepmgr *aem)
{
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	struct ar5211_base_eep_hdr *base = &eep->base;
	struct ar5211_modal_eep_hdr *modal_a = &eep->modal_a;
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
	modal_a->turbo_maxtxpwr_2w = MS(word, AR5211_EEP_TURBO5_MAXPWR);
	base->devtype = MS(word, AR5211_EEP_DEVTYPE);
	base->rfkill_en = !!(word & AR5211_EEP_RFKILL_EN);
	base->turbo5_dis = !!(word & AR5211_EEP_TURBO5_DIS);

	if (base->version >= AR5211_EEP_VER_3_3) {
		eep_5211_fill_headers_33(aem);
		eep_5211_parse_modal_a(aem, AR5211_EEP_MODAL_A_33);
		eep_5211_parse_modal_b(aem, AR5211_EEP_MODAL_B_33);
		eep_5211_parse_modal_g(aem, AR5211_EEP_MODAL_G_33);
	} else if (base->version >= AR5211_EEP_VER_3_0) {
		eep_5211_fill_headers_30(aem);
		eep_5211_parse_modal_a(aem, AR5211_EEP_MODAL_A_30);
		eep_5211_parse_modal_b(aem, AR5211_EEP_MODAL_B_30);
		eep_5211_parse_modal_g(aem, AR5211_EEP_MODAL_G_30);
		if (base->version >= AR5211_EEP_VER_3_1)
			eep_5211_parse_modal_ext_31(aem);
	}
}

/* Used only for map0 PD calibration data */
static void eep_5211_decode_xpd_gain(uint8_t eep_val,
				     struct eep_5211_pdcal_param *pdcp)
{
	static const int8_t gains[] = {-1, -1, -1, -1, -1, -1, -1, 18, -1, -1,
				       -1, 12, -1,  6,  0, -1, -1, -1, -1, -1};

	if (eep_val < ARRAY_SIZE(gains) && gains[eep_val] != -1) {
		pdcp->gains[0] = gains[eep_val];
	} else {
		printf("Unknown xPD gain code 0x%02x, use 6 dB\n", eep_val);
		pdcp->gains[0] = 6;
	}
	pdcp->ngains = 1;
}

/* Use for map1 & map2 PD calibration data */
static void eep_5211_parse_xpd_gain(uint8_t eep_val, const int8_t *map,
				    struct eep_5211_pdcal_param *pdcp)
{
	int i, j;

	for (i = 0, j = 0; i < AR5211_MAX_PDCAL_GAINS; ++i) {
		if (!(eep_val & (1 << i)))
			continue;
		pdcp->gains[j++] = map[i];
	}
	pdcp->ngains = j;
}

static void eep_5211_count_pdcal_piers(const uint8_t *piers, int *npiers,
				       int maxpiers)
{
	int i = 0;

	while (piers[i] && ++i < maxpiers);
	*npiers = i;
}

static void eep_5211_parse_pdcal_piers_30(struct atheepmgr *aem,
					  struct eep_bit_stream *ebs,
					  uint8_t *piers, int *npiers,
					  int maxpiers)
{
	int i = 0;

	do {
		piers[i] = EEP_GET_MSB(7);
		piers[i] = FBIN_30_TO_33(piers[i], 0);
	} while (piers[i] && ++i < maxpiers);
	*npiers = i;
	for (++i; i < maxpiers; ++i)
		EEP_GET_LSB(8);		/* Read leftover */
	EEP_GET_MSB(10);	/* Skip unused bits */
}

static void eep_5211_parse_pdcal_piers_33(struct atheepmgr *aem,
					  struct eep_bit_stream *ebs,
					  uint8_t *piers, int *npiers,
					  int maxpiers)
{
	int i = 0;

	do {
		piers[i] = EEP_GET_MSB(8);
	} while (piers[i] && ++i < maxpiers);
	*npiers = i;
	for (++i; i < maxpiers; ++i)
		EEP_GET_LSB(8);		/* Read leftover */
}

static void eep_5211_parse_pdcal_piers_40(struct atheepmgr *aem,
					  struct eep_bit_stream *ebs,
					  uint8_t *piers, int *npiers,
					  int maxpiers)
{
	int i = 0;

	do {
		piers[i] = EEP_GET_LSB(8);
	} while (piers[i] && ++i < maxpiers);
	*npiers = i;
	for (++i; i < maxpiers; ++i)
		EEP_GET_LSB(8);		/* Read leftover */
}

/**
 * Map 0 format does not store VPD value of each point, instead it stores
 * min and max VPD values and output tx power levels measured at predefined
 * VPD values, which are expressed as percents. E.g.:
 *   VPD0 = VPDmin + 0.00 * (VPDmax - VPDmin) // 0%
 *   VPD1 = VPDmin + 0.10 * (VPDmax - VPDmin) // 10%
 *   VPD2 = VPDmin + 0.20 * (VPDmax - VPDmin) // 20%
 *   ...
 *   VPD11 = VPDmin + 1.00 * (VPDmax - VPDmin) // 100%
 */
static void eep_5211_parse_pdcal_data_map0(struct atheepmgr *aem,
					   struct eep_bit_stream *ebs,
					   struct eep_5211_pdcal_param *pdcp,
					   struct ar5211_pier_pdcal *pdcal)
{
#define ICEPTS_NUM	11
	static const unsigned int icepts_vpd_percent[2][ICEPTS_NUM] = {
		{0,  5, 10, 20, 30, 50, 70, 85, 90, 95, 100},	/*  < v3.0 */
		{0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100}	/* >= v3.2 */
	};
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	const unsigned int *vp;
	unsigned int vpd_min, vpd_max;
	int16_t *pwr;
	uint8_t *vpd;
	int i, j;

	vp = eep->base.version < AR5211_EEP_VER_3_2 ? icepts_vpd_percent[0] :
						      icepts_vpd_percent[1];

	for (i = 0; i < pdcp->npiers; ++i) {
		pwr = pdcal[i].pwr[0];
		vpd = pdcal[i].vpd[0];
		vpd_max = EEP_GET_MSB(6);
		vpd_min = EEP_GET_MSB(6);
		for (j = 0; j < ICEPTS_NUM; ++j) {
			pwr[j] = EEP_GET_MSB(6) * 2;	/* 0.5 dB -> 0.25 dB */
			vpd[j] = vpd_min + vp[j] * (vpd_max - vpd_min) / 100;
		}
		pdcp->nicepts[0] = j;
		EEP_GET_MSB(2);				/* Skip unused bits */
	}

#undef ICEPTS_NUM
}

static void eep_5211_parse_pdcal_map0(struct atheepmgr *aem,
				      struct eep_bit_stream *ebs)
{
#define F2B(__freq)	FREQ2FBIN(__freq, 1)
	static const uint8_t piers_b[] = {F2B(2412), F2B(2447), F2B(2484)};
	static const uint8_t piers_g[] = {F2B(2312), F2B(2412), F2B(2484)};
#undef F2B
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	struct eep_5211_pdcal_param *pdcp;

	pdcp = &emp->param.pdcal_a;
	if (eep->base.version >= AR5211_EEP_VER_3_3)
		eep_5211_parse_pdcal_piers_33(aem, ebs, eep->pdcal_piers_a,
					      &pdcp->npiers,
					      AR5211_NUM_PDCAL_PIERS_A);
	else
		eep_5211_parse_pdcal_piers_30(aem, ebs, eep->pdcal_piers_a,
					      &pdcp->npiers,
					      AR5211_NUM_PDCAL_PIERS_A);
	pdcp->piers = eep->pdcal_piers_a;
	eep_5211_decode_xpd_gain(eep->modal_a.xpd_gain, pdcp);
	eep_5211_parse_pdcal_data_map0(aem, ebs, pdcp, eep->pdcal_data_a);

	pdcp = &emp->param.pdcal_b;
	pdcp->piers = piers_b;		/* Fixed piers */
	pdcp->npiers = ARRAY_SIZE(piers_b);
	eep_5211_decode_xpd_gain(eep->modal_b.xpd_gain, pdcp);
	eep_5211_parse_pdcal_data_map0(aem, ebs, pdcp, eep->pdcal_data_b);

	pdcp = &emp->param.pdcal_g;
	pdcp->piers = piers_g;		/* Fixed piers */
	pdcp->npiers = ARRAY_SIZE(piers_g);
	eep_5211_decode_xpd_gain(eep->modal_g.xpd_gain, pdcp);
	eep_5211_parse_pdcal_data_map0(aem, ebs, pdcp, eep->pdcal_data_g);
}

static void eep_5211_parse_pdcal_data_map1(struct atheepmgr *aem,
					   struct eep_bit_stream *ebs,
					   struct eep_5211_pdcal_param *pdcp,
					   struct ar5211_pier_pdcal *pdcal)
{
	static const uint8_t hi_xpd_gain_vpd[3] = {20, 35, 63};	/* Fixed */
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	int i, j;
	uint8_t vpd_d[4];	/* VPD deltas */

	/**
	 * NB: storage format always contains data for 2 xPD gains, even if
	 * only one is in use. So parse data for both gains and dumping code
	 * will ignore data for higher gain if only one gain is in use.
	 */
	for (i = 0; i < pdcp->npiers; ++i) {
		pdcp->nicepts[0] = 4;
		for (j = 0; j < pdcp->nicepts[0]; ++j)
			pdcal[i].pwr[0][j] = (int8_t)EEP_GET_LSB(8);

		for (j = 1; j < 4; ++j)
			vpd_d[j] = EEP_GET_LSB(5);
		EEP_GET_LSB(1);			/* Skip unused bit */

		pdcp->nicepts[1] = 3;
		for (j = 0; j < pdcp->nicepts[1]; ++j)
			pdcal[i].pwr[1][j] = (int8_t)EEP_GET_LSB(8);

		if (eep->base.version < AR5211_EEP_VER_4_3) {
			pdcal[i].vpd[0][0] = 1;	/* Fixed VPD value */
			EEP_GET_LSB(8);		/* Skip max power value */
		} else {
			pdcal[i].vpd[0][0] = EEP_GET_LSB(6);
			EEP_GET_LSB(2);		/* Skip unused bits */
		}
		for (j = 1; j < 4; ++j)
			pdcal[i].vpd[0][j] = pdcal[i].vpd[0][j - 1] + vpd_d[j];

		for (j = 0; j < ARRAY_SIZE(hi_xpd_gain_vpd); ++j)
			pdcal[i].vpd[1][j] = hi_xpd_gain_vpd[j];
	}
}

static void eep_5211_parse_pdcal_map1(struct atheepmgr *aem,
				      struct eep_bit_stream *ebs)
{
	static const int8_t gains_map[] = {0, 6, 12, 18};
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	struct eep_5211_pdcal_param *pdcp;

	if (eep->base.amode_en) {
		pdcp = &emp->param.pdcal_a;
		eep_5211_parse_pdcal_piers_40(aem, ebs, eep->pdcal_piers_a,
					      &pdcp->npiers,
					      AR5211_NUM_PDCAL_PIERS_A);
		pdcp->piers = eep->pdcal_piers_a;
		eep_5211_parse_xpd_gain(eep->modal_a.xpd_gain, gains_map, pdcp);
		eep_5211_parse_pdcal_data_map1(aem, ebs, pdcp,
					       eep->pdcal_data_a);
	}

	if (eep->base.bmode_en) {
		pdcp = &emp->param.pdcal_b;
		eep_5211_count_pdcal_piers(eep->modal_b.cal_piers,
					   &pdcp->npiers,
					   ARRAY_SIZE(eep->modal_b.cal_piers));
		pdcp->piers = eep->modal_b.cal_piers;
		eep_5211_parse_xpd_gain(eep->modal_b.xpd_gain, gains_map, pdcp);
		eep_5211_parse_pdcal_data_map1(aem, ebs, pdcp,
					       eep->pdcal_data_b);
	}

	if (eep->base.gmode_en) {
		pdcp = &emp->param.pdcal_g;
		eep_5211_count_pdcal_piers(eep->modal_g.cal_piers,
					   &pdcp->npiers,
					   ARRAY_SIZE(eep->modal_g.cal_piers));
		pdcp->piers = eep->modal_g.cal_piers;
		eep_5211_parse_xpd_gain(eep->modal_g.xpd_gain, gains_map, pdcp);
		eep_5211_parse_pdcal_data_map1(aem, ebs, pdcp,
					       eep->pdcal_data_g);
	}
}

static void eep_5211_parse_pdcal_data_map2(struct atheepmgr *aem,
					   struct eep_bit_stream *ebs,
					   struct eep_5211_pdcal_param *pdcp,
					   struct ar5211_pier_pdcal *pdcal)
{
	int maxk;
	int i, j, k;
	int16_t *pwr;
	uint8_t *vpd;

	for (i = 0; i < pdcp->npiers; ++i) {
		for (j = pdcp->ngains - 1; j >= 0; --j) {
			maxk = j == 0 ? 5 : 4;
			pwr = pdcal[i].pwr[j];
			vpd = pdcal[i].vpd[j];
			pwr[0] = EEP_GET_LSB(5) * 4;	/* dB -> 0.25 dB */
			vpd[0] = EEP_GET_LSB(7);
			for (k = 1; k < maxk; ++k) {
				pwr[k] = pwr[k - 1] + EEP_GET_LSB(4) * 2;
				vpd[k] = vpd[k - 1] + EEP_GET_LSB(6);
			}
			pdcp->nicepts[j] = maxk;
		}
		EEP_GET_LSB(ebs->havebits);	/* Skip till word boundary */
	}
}

static void eep_5211_parse_pdcal_map2(struct atheepmgr *aem,
				      struct eep_bit_stream *ebs)
{
	/* Following array is approximation of {x0.5, x1, x2, x4} gains */
	static const int8_t gains_map[] = {-6, 0, 6, 12};
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	struct eep_5211_pdcal_param *pdcp;

	if (eep->base.amode_en) {
		pdcp = &emp->param.pdcal_a;
		eep_5211_parse_pdcal_piers_40(aem, ebs, eep->pdcal_piers_a,
					      &pdcp->npiers,
					      AR5211_NUM_PDCAL_PIERS_A);
		pdcp->piers = eep->pdcal_piers_a;
		eep_5211_parse_xpd_gain(eep->modal_a.xpd_gain, gains_map, pdcp);
		eep_5211_parse_pdcal_data_map2(aem, ebs, pdcp,
					       eep->pdcal_data_a);
	}

	if (eep->base.bmode_en) {
		pdcp = &emp->param.pdcal_b;
		eep_5211_parse_pdcal_piers_40(aem, ebs, eep->pdcal_piers_b,
					      &pdcp->npiers,
					      AR5211_NUM_PDCAL_PIERS_B);
		pdcp->piers = eep->pdcal_piers_b;
		eep_5211_parse_xpd_gain(eep->modal_b.xpd_gain, gains_map, pdcp);
		eep_5211_parse_pdcal_data_map2(aem, ebs, pdcp,
					       eep->pdcal_data_b);
	}

	if (eep->base.gmode_en) {
		pdcp = &emp->param.pdcal_g;
		eep_5211_parse_pdcal_piers_40(aem, ebs, eep->pdcal_piers_g,
					      &pdcp->npiers,
					      AR5211_NUM_PDCAL_PIERS_G);
		pdcp->piers = eep->pdcal_piers_g;
		eep_5211_parse_xpd_gain(eep->modal_g.xpd_gain, gains_map, pdcp);
		eep_5211_parse_pdcal_data_map2(aem, ebs, pdcp,
					       eep->pdcal_data_g);
	}
}

static void eep_5211_parse_pdcal(struct atheepmgr *aem)
{
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct eep_bit_stream __ebs, *ebs = &__ebs;

	memset(ebs, 0x00, sizeof(*ebs));
	ebs->eep_off = emp->param.pdcal_off;

	if (emp->param.eepmap == 2)
		eep_5211_parse_pdcal_map2(aem, ebs);
	else if (emp->param.eepmap == 1)
		eep_5211_parse_pdcal_map1(aem, ebs);
	else if (emp->param.eepmap == 0)
		eep_5211_parse_pdcal_map0(aem, ebs);
}

static void eep_5211_parse_tgtpwr_set(struct atheepmgr *aem,
				      struct eep_bit_stream *ebs,
				      struct ar5211_chan_tgtpwr *tgtpwr,
				      int maxchans, int is_2g)
{
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	int i, j;

	for (i = 0; i < maxchans; ++i) {
		if (eep->base.version < AR5211_EEP_VER_3_3) {
			tgtpwr[i].chan = EEP_GET_MSB(7);
			tgtpwr[i].chan = FBIN_30_TO_33(tgtpwr[i].chan, is_2g);
		} else {
			tgtpwr[i].chan = EEP_GET_MSB(8);
		}
		for (j = 0; j < AR5211_NUM_TGTPWR_RATES; ++j)
			tgtpwr[i].pwr[j] = EEP_GET_MSB(6);
		if (eep->base.version < AR5211_EEP_VER_3_3)
			EEP_GET_MSB(1);		/* Skip unused bit */
	}
}

static void eep_5211_parse_tgtpwr(struct atheepmgr *aem)
{
	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	struct eep_bit_stream __ebs, *ebs = &__ebs;

	memset(ebs, 0x00, sizeof(*ebs));
	ebs->eep_off = emp->param.tgtpwr_off;

	eep_5211_parse_tgtpwr_set(aem, ebs, eep->tgtpwr_a,
				  ARRAY_SIZE(eep->tgtpwr_a), 0);
	eep_5211_parse_tgtpwr_set(aem, ebs, eep->tgtpwr_b,
				  ARRAY_SIZE(eep->tgtpwr_b), 1);
	eep_5211_parse_tgtpwr_set(aem, ebs, eep->tgtpwr_g,
				  ARRAY_SIZE(eep->tgtpwr_g), 1);
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

static bool eep_5211_load_eeprom(struct atheepmgr *aem, bool raw)
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
		if (aem->verbose)
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

	eep_5211_parse_pdcal(aem);
	eep_5211_parse_tgtpwr(aem);

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

	EEP_PRINT_SECT_NAME("Chip init data");

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

	printf("\nCustomer Data in hex:\n");
	hexdump_print(eep->cust_data, sizeof(eep->cust_data));

	printf("\n");

#undef PR
}

static void eep_5211_dump_modal(struct atheepmgr *aem)
{
#define _MODE_A		0x01
#define _MODE_B		0x02
#define _MODE_G		0x04
#define _MODE_AG	(_MODE_A | _MODE_G)
#define _MODE_BG	(_MODE_B | _MODE_G)
#define _MODE_ABG	(_MODE_A | _MODE_B | _MODE_G)
#define _PR_BEGIN(_token)					\
		printf("%-24s:", _token);			\
		curpos = 0;
#define _PR_END()						\
		printf("\n");
#define _PR_VAL(_fpos, _modes, _mode, _fmt, ...)		\
	if (_modes & _mode) {					\
		curpos += printf("%*s", _fpos - curpos, "");	\
		curpos += printf(_fmt, ## __VA_ARGS__);		\
	}
#define _PR_FIELD(_modes, _token, _fmt, _field)			\
	do {							\
		_PR_BEGIN(_token);				\
		_PR_VAL(6, _modes, _MODE_A, _fmt, eep->modal_a._field);\
		_PR_VAL(20, _modes, _MODE_B, _fmt, eep->modal_b._field);\
		_PR_VAL(34, _modes, _MODE_G, _fmt, eep->modal_g._field);\
		_PR_END();					\
	} while (0)
#define PR_DEC(_modes, _token, _field)				\
		_PR_FIELD(_MODE_ ## _modes, _token, "% d", _field)
#define PR_HEX(_modes, _token, _field)				\
		_PR_FIELD(_MODE_ ## _modes, _token, " 0x%02X", _field)
#define PR_FLOAT(_modes, _token, _field)			\
		_PR_FIELD(_MODE_ ## _modes, _token, "% 3.1f", _field)
#define PR_STR(_modes, _token, _field)				\
		_PR_FIELD(_MODE_ ## _modes, _token, " %s", _field)

	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	char tok[0x20];
	int i, curpos;

	EEP_PRINT_SECT_NAME("EEPROM Modal Header");

	printf("%24s %7s%-7s%7s%-7s%7s%s\n\n", "", "", ".11a", "", ".11b", "", ".11g");

	for (i = 0; i < ARRAY_SIZE(eep->modal_a.ant_ctrl); ++i) {
		snprintf(tok, sizeof(tok), "Ant control #%-2u", i);
		PR_HEX(ABG, tok, ant_ctrl[i]);
	}
	PR_DEC(ABG, "Switch settling time", sw_settle_time);

	if (eep->base.version >= AR5211_EEP_VER_4_0) {
		PR_DEC(AG, "I/Q calibration I", iq_cal_i);
		PR_DEC(AG, "I/Q calibration Q", iq_cal_q);
	}
	PR_DEC(ABG, "Tx/Rx attenuation, dB", txrx_atten);
	if (eep->base.version >= AR5211_EEP_VER_4_1)
		PR_DEC(ABG, "Rx/Tx margin, dB", rxtx_margin);

	PR_DEC(ABG, "Thresh62", thresh62);
	PR_DEC(ABG, "NF threshold, dBm", nfthresh);
	PR_DEC(ABG, "xLNA gain, dB", xlna_gain);
	if (eep->base.version >= AR5211_EEP_VER_3_3)
		PR_DEC(ABG, "FalseDetect backoff", false_detect_backoff);

	_PR_BEGIN("PA output bias");
	_PR_VAL(6, _MODE_ABG, _MODE_A, "{%d, %d, %d, %d}",
		eep->modal_a.pa_ob[0], eep->modal_a.pa_ob[1],
		eep->modal_a.pa_ob[2], eep->modal_a.pa_ob[3]);
	_PR_VAL(20, _MODE_ABG, _MODE_B, "{%d}", eep->modal_b.pa_ob[0]);
	_PR_VAL(34, _MODE_ABG, _MODE_G, "{%d}", eep->modal_g.pa_ob[0]);
	_PR_END();
	_PR_BEGIN("PA drive bias");
	_PR_VAL(6, _MODE_ABG, _MODE_A, "{%d, %d, %d, %d}",
		eep->modal_a.pa_db[0], eep->modal_a.pa_db[1],
		eep->modal_a.pa_db[2], eep->modal_a.pa_db[3]);
	_PR_VAL(20, _MODE_ABG, _MODE_B, "{%d}", eep->modal_b.pa_db[0]);
	_PR_VAL(34, _MODE_ABG, _MODE_G, "{%d}", eep->modal_g.pa_db[0]);
	_PR_END();
	if (eep->base.version >= AR5211_EEP_VER_4_0)
		PR_STR(AG, "Fixed bias", fixed_bias ? "fixed" : "auto");
	if (eep->base.version >= AR5211_EEP_VER_3_1) {
		PR_DEC(BG, "2.4 GHz PA output bias", pa_ob_2ghz);
		PR_DEC(BG, "2.4 GHz PA drive bias", pa_db_2ghz);
	}

	PR_DEC(ABG, "Tx End to xLNA On", tx_end_to_xlna_on);
	PR_DEC(ABG, "Tx End to xPA Off", tx_end_to_xpa_off);
	PR_DEC(ABG, "Tx Frame to xPA On", tx_frame_to_xpa_on);

	PR_FLOAT(ABG, "ADC desired size, dBm", adc_desired_size / 2.0);
	PR_FLOAT(ABG, "PGA desired size, dBm", pga_desired_size / 2.0);
	PR_HEX(ABG, "xPD gain", xpd_gain);
	PR_STR(ABG, "xPD type", xpd ? "external" : "internal");
	if (eep->base.version >= AR5211_EEP_VER_3_4)
		PR_HEX(ABG, "xPD initial gain", pd_gain_init);

	if (eep->base.version >= AR5211_EEP_VER_3_4)
		PR_FLOAT(G, "CCK/OFDM pwr delta, dBm",
			 cck_ofdm_pwr_delta / 10.0);
	if (eep->base.version >= AR5211_EEP_VER_4_2)
		PR_DEC(G, "CCK/OFDM gain delta, dB", cck_ofdm_gain_delta);
	if (eep->base.version >= AR5211_EEP_VER_4_6)
		PR_FLOAT(G, "Ch14 filt CCK delta, dBm",
			 ch14_filter_cck_delta / 10.0);

	snprintf(tok, sizeof(tok), "Turbo maxtxpwr 2W, dBm");
	if (eep->base.version >= AR5211_EEP_VER_4_0)
		PR_FLOAT(AG, tok, turbo_maxtxpwr_2w / 2.0);
	else
		PR_FLOAT(A, tok, turbo_maxtxpwr_2w / 2.0);

	if (eep->base.version >= AR5211_EEP_VER_5_0) {
		PR_DEC(AG, "Turbo sw settling time", turbo_sw_settle_time);
		PR_DEC(AG, "Turbo Tx/Rx attenuation", turbo_txrx_atten);
		PR_DEC(AG, "Turbo Rx/Tx margin, dB", turbo_rxtx_margin);
		PR_FLOAT(AG, "Turbo ADC des. size, dBm",
			 turbo_adc_desired_size / 2.0);
		PR_FLOAT(AG, "Turbo PGA des. size, dBm",
			 turbo_pga_desired_size / 2.0);
	}

	snprintf(tok, sizeof(tok), "XR target power, dBm");
	if (eep->base.version >= AR5211_EEP_VER_4_0)
		PR_FLOAT(AG, tok, xr_tgt_pwr / 2.0);
	else if (eep->base.version >= AR5211_EEP_VER_3_3)
		PR_FLOAT(A, tok, xr_tgt_pwr / 2.0);

#undef PR_STR
#undef PR_FLT
#undef PR_DEC
#undef PR_HEX
#undef _PR_FIELD
#undef _PR_VAL
#undef _PR_END
#undef _PR_BEGIN
#undef _MODE_A
#undef _MODE_B
#undef _MODE_G
#undef _MODE_AG
#undef _MODE_BG
#undef _MODE_ABG
}

static void eep_5211_dump_pdcal_pier(const int8_t *gains, int ngains,
				     const struct ar5211_pier_pdcal *pdcal,
				     const int *nicepts)
{
	struct {
		int16_t pwr;
		uint8_t vpd[AR5211_MAX_PDCAL_GAINS];
	} merged[AR5211_MAX_PDCAL_ICEPTS * AR5211_MAX_PDCAL_GAINS];
	int gii[AR5211_MAX_PDCAL_GAINS];	/* Array of indexes for merge */
	int gain, pwr, npwr;			/* Indexes */
	int16_t pwrmin;

	/* Merge calibration per-gain power lists to filter duplicates */
	memset(merged, 0xff, sizeof(merged));
	memset(gii, 0x00, sizeof(gii));
	for (pwr = 0; pwr < ARRAY_SIZE(merged); ++pwr) {
		pwrmin = INT16_MAX;
		for (gain = 0; gain < ngains; ++gain) {
			if (gii[gain] >= nicepts[gain])
				continue;
			if (pdcal->pwr[gain][gii[gain]] < pwrmin)
				pwrmin = pdcal->pwr[gain][gii[gain]];
		}
		if (pwrmin == INT16_MAX)
			break;
		merged[pwr].pwr = pwrmin;
		for (gain = 0; gain < ngains; ++gain) {
			if (gii[gain] >= nicepts[gain] ||
			    pdcal->pwr[gain][gii[gain]] != pwrmin)
				continue;
			merged[pwr].vpd[gain] = pdcal->vpd[gain][gii[gain]];
			gii[gain]++;
		}
	}
	npwr = pwr;

	/* Print merged data */
	printf("     Tx Power, dBm:");
	for (pwr = 0; pwr < npwr; ++pwr)
		printf(" %5.2f", merged[pwr].pwr / 4.0);
	printf("\n");
	printf("    ---------------");
	for (pwr = 0; pwr < npwr; ++pwr)
		printf(" -----");
	printf("\n");
	for (gain = 0; gain < ngains; ++gain) {
		printf("   % 3d dB gain VPD:", gains[gain]);
		for (pwr = 0; pwr < npwr; ++pwr) {
			if (merged[pwr].vpd[gain] == 0xff)
				printf("      ");
			else
				printf("   %3u", merged[pwr].vpd[gain]);
		}
		printf("\n");
	}
}

static void eep_5211_dump_pdcal(const struct eep_5211_pdcal_param *pdcp,
				const struct ar5211_pier_pdcal *pdcal,
				int is_2g)
{
	int pier;

	for (pier = 0; pier < pdcp->npiers; ++pier) {
		printf("  %4u MHz:\n", FBIN2FREQ(pdcp->piers[pier], is_2g));
		eep_5211_dump_pdcal_pier(pdcp->gains, pdcp->ngains,
					 &pdcal[pier], pdcp->nicepts);
		printf("\n");
	}
}

static void eep_5211_dump_tgtpwr(const struct ar5211_chan_tgtpwr *tgtpwr,
				 int maxchans, const char * const rates[],
				 int is_2g)
{
#define MARGIN		"    "

	int nchans, i, j;

	printf(MARGIN "%10s, MHz:", "Freq");
	for (j = 0; j < maxchans; ++j) {
		if (!tgtpwr[j].chan)
			break;
		printf("  %4u", FBIN2FREQ(tgtpwr[j].chan, is_2g));
	}
	nchans = j;
	printf("\n");
	printf(MARGIN "----------------");
	for (j = 0; j < nchans; ++j)
		printf("  ----");
	printf("\n");

	for (i = 0; i < AR5211_NUM_TGTPWR_RATES; ++i) {
		printf(MARGIN "%10s, dBm:", rates[i]);
		for (j = 0; j < nchans; ++j)
			printf("  %4.1f", tgtpwr[j].pwr[i] / 2.0);
		printf("\n");
	}

#undef MARGIN
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
#define PR_PD_CAL(__suf, __mode, __is_2g)				\
	if (base-> __mode ## mode_en) {					\
		EEP_PRINT_SUBSECT_NAME("Mode 802.11" __suf " per-freq PD cal. data");\
		eep_5211_dump_pdcal(&emp->param.pdcal_ ## __mode,	\
				    eep->pdcal_data_ ## __mode, __is_2g);\
		printf("\n");						\
	}
#define PR_TGT_PWR(__suf, __mode, __rates, __is_2g)			\
	if (base-> __mode ## mode_en) {					\
		EEP_PRINT_SUBSECT_NAME("Mode 802.11" __suf " per-rate target power");\
		eep_5211_dump_tgtpwr(eep->tgtpwr_ ## __mode,		\
				     ARRAY_SIZE(eep->tgtpwr_ ## __mode),\
				     __rates, __is_2g);			\
		printf("\n");						\
	}

	struct eep_5211_priv *emp = aem->eepmap_priv;
	struct ar5211_eeprom *eep = &emp->eep;
	struct ar5211_base_eep_hdr *base = &eep->base;

	EEP_PRINT_SECT_NAME("EEPROM Power Info");

	PR_PD_CAL("a", a, 0);
	PR_PD_CAL("b", b, 1);
	PR_PD_CAL("g", g, 1);

	PR_TGT_PWR("a", a, eep_rates_ofdm, 0);
	PR_TGT_PWR("b", b, eep_rates_cck, 1);
	PR_TGT_PWR("g", g, eep_rates_ofdm, 1);

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
	.chip_regs = {
		.srev = 0x4020,
	},
	.priv_data_sz = sizeof(struct eep_5211_priv),
	.eep_buf_sz = AR5211_SIZE_MAX,
	.load_eeprom = eep_5211_load_eeprom,
	.check_eeprom = eep_5211_check,
	.dump = {
		[EEP_SECT_INIT] = eep_5211_dump_init_data,
		[EEP_SECT_BASE] = eep_5211_dump_base,
		[EEP_SECT_MODAL] = eep_5211_dump_modal,
		[EEP_SECT_POWER] = eep_5211_dump_power,
	},
	.update_eeprom = eep_5211_update_eeprom,
	.params_mask = BIT(EEP_UPDATE_MAC)
#ifdef CONFIG_I_KNOW_WHAT_I_AM_DOING
		| BIT(EEP_ERASE_CTL)
#endif
	,
};
