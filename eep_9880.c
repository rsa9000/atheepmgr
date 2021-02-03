/*
 * Copyright (c) 2020-2021 Sergey Ryazanov <ryazanov.s.a@gmail.com>
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
#include "eep_9880.h"
#include "eep_9880_templates.h"

static const uint8_t eep_9880_otp_magic[2] = {0xaa, 0x55};

struct eep_9880_priv {
	int curr_ref_tpl;		/* Current reference EEPROM template */
	struct qca9880_eeprom eep;
};

#define QCA9880_TEMPLATE_DESC(__name, __tpl)	\
	{ qca9880_tpl_ver_ ## __tpl, __name, &qca9880_ ## __tpl }

static const struct eeptemplate eep_9880_templates[] = {
	QCA9880_TEMPLATE_DESC("CUS223", cus223),
	QCA9880_TEMPLATE_DESC("XB140", xb140),
	{ 0, NULL }
};

static const uint8_t *qca9880_template_find_by_id(int id)
{
	const struct eeptemplate *tpl;

	for (tpl = eep_9880_templates; tpl->name; ++tpl)
		if (tpl->id == id)
			break;

	return tpl->data;
}

static void eep_9880_proc_otp_caldata(struct atheepmgr *aem,
				      const uint8_t *data, int len)
{
	struct eep_9880_priv *emp = aem->eepmap_priv;
	struct ar9300_comp_hdr hdr;
	uint16_t cksum, _cksum;

	ar9300_comp_hdr_unpack(data, &hdr);
	if (aem->verbose)
		printf("Found block at %x: comp=%d ref=%d length=%d major=%d minor=%d\n",
		       0, hdr.comp, hdr.ref, hdr.len, hdr.maj, hdr.min);

	data += sizeof(AR9300_COMP_HDR_LEN);
	len -= AR9300_COMP_HDR_LEN + sizeof(cksum);
	if (hdr.len > len) {
		if (aem->verbose)
			printf("Caldata block length greater then OTP stream length\n");
		return;
	}

	cksum = ar9300_comp_cksum(data, hdr.len);
	_cksum = data[hdr.len + 0] | (data[hdr.len + 1] << 8);
	if (cksum != _cksum) {
		if (aem->verbose)
			printf("Bad caldata block checksum (got 0x%04x, expect 0x%04x)\n",
			       cksum, _cksum);
		return;
	}

	ar9300_compress_decision(aem, 0, &hdr, aem->unpacked_buf, data,
				 sizeof(emp->eep), &emp->curr_ref_tpl,
				 qca9880_template_find_by_id);
}

static struct eep_9880_otp_str_desc {
	const char *name;
	void (*proc)(struct atheepmgr *aem, const uint8_t *data, int len);
} eep_9880_otp_streams[] = {
	[QCA9880_OTP_STR_TYPE_CALDATA] = {
		"calibration data", eep_9880_proc_otp_caldata
	},
};

static bool eep_9880_load_blob(struct atheepmgr *aem)
{
	const int data_size = sizeof(struct qca9880_eeprom);
	struct eep_9880_priv *emp = aem->eepmap_priv;
	int res;

	if (aem->con->blob->getsize(aem) < data_size)
		return false;
	res = aem->con->blob->read(aem, aem->eep_buf, data_size);
	if (res != data_size) {
		fprintf(stderr, "Unable to read calibration data blob\n");
		return false;
	}

	memcpy(&emp->eep, aem->eep_buf, sizeof(emp->eep));

	aem->eep_len = (data_size + 1) / 2;

	return true;
}

static bool eep_9880_load_otp(struct atheepmgr *aem)
{
	struct eep_9880_priv *emp = aem->eepmap_priv;
	struct qca9880_eeprom *eep;
	uint8_t *buf = (uint8_t *)aem->eep_buf;	/* Use as an array of bytes */
	unsigned int addr, end_mark_seen;
	uint8_t strcode;
	uint8_t *p, *s;

	if (!OTP_ENABLE()) {
		fprintf(stderr, "Unable to enable chip OTP memory access");
		return false;
	}

	for (addr = 0; addr < QCA9880_OTP_SIZE; ++addr) {
		if (!OTP_READ(addr, &buf[addr])) {
			fprintf(stderr, "Unable to read OTP at 0x%04x\n", addr);
			goto exit;
		}
	}

	/**
	 * Check OTP magic. Do not have macro to work with big-endian values,
	 * so check byte by byte.
	 */
	if (memcmp(&buf[QCA9880_OTP_MAGIC_OFFSET], eep_9880_otp_magic,
	           sizeof(eep_9880_otp_magic)) != 0) {
		if (aem->verbose > 1)
			printf("Invalid OTP magic 0x%02X%02X, expected value 0x%02X%02X\n",
			       buf[QCA9880_OTP_MAGIC_OFFSET + 0],
			       buf[QCA9880_OTP_MAGIC_OFFSET + 1],
			       eep_9880_otp_magic[0], eep_9880_otp_magic[1]);
		goto exit;
	}

	emp->curr_ref_tpl = -1;	/* Reset reference template */

	/**
	 * Now we are ready to parse OTP memory content.
	 *
	 * OTP data start from a fixed size header that is followed by a several
	 * data containers of a variable size. Data containers are stored in a
	 * stream like format: each container (stream) begins with a predefined
	 * one octent lenth marker, that is followed by a stream header and
	 * data, and then stream is finished by an end marker of two identical
	 * octets. Sream length are never stored, instead the stream length
	 * should be calculated as a difference between the 'end' and 'begin'
	 * markers.
	 *
	 * Each (begin/end) marker consists of a constant (high nibble) and
	 * variable (low nibble) parts. Constant part is different for begin
	 * and for end markers, but stay the same for different streams. While
	 * variable part should be identical for begin and for end markers of
	 * item stream, but could differ from stream to stream. Looks like
	 * the variable part of marker is used to avoid false positivie end
	 * marker detection. I.e. OTP writing sofware selects value for the
	 * variable part in a such way that avoids appearing of double end
	 * marker in the stream data. But we should not care, since we should
	 * just parse this. So parser extract a value of the variable part
	 * (lets call it a stream code) of the begin marker and then looking
	 * for the end marker by checking each next octet of OTP for the
	 * constant part of the end marker and for variable part (stream code)
	 * that was extracted from the begin marker.
	 */
	strcode = 0xff;
	s = NULL;	/* Uninit. usage is impossible, but make gcc happy */
	for (p = buf+QCA9880_OTP_HEADER_SIZE; p < buf+QCA9880_OTP_SIZE; ++p) {
		if (strcode == 0xff) {		/* Not inside OTP stream */
			if (*p == 0x00)		/* Unused area begin */
				break;
			if (!QCA9880_OTP_STR_MARK_IS_BEGIN(*p)) {
				fprintf(stderr, "Invalid OTP stream begin marker 0x%02x at 0x%04x\n",
					*p, (unsigned int)(p - buf));
				goto exit;
			}
			strcode = QCA9880_OTP_STR_MARK_CODE(*p);
			end_mark_seen = 0;
			s = p;			/* Catch stream begin */
		} else if (!QCA9880_OTP_STR_MARK_IS_END(*p) ||
			   strcode != QCA9880_OTP_STR_MARK_CODE(*p)) {
			end_mark_seen = 0;
		} else if (!end_mark_seen) {	/* Got first 'end' mark */
			end_mark_seen = 1;
		} else {			/* Got second 'end' mark */
			const struct eep_9880_otp_str_desc *strdesc;
			const struct qca9880_otp_str *str = (void *)(s + 1);
			unsigned int len = p - s - 2;	/* Exclude markers */

			addr = s - buf;
			if (len < sizeof(*str)) {
				fprintf(stderr, "Too short OTP stream raw data length %u byte(s) at 0x%04x\n",
					len, addr);
				goto exit;
			}

			strdesc = str->type >= ARRAY_SIZE(eep_9880_otp_streams) ?
				  NULL : &eep_9880_otp_streams[str->type];

			if (aem->verbose > 1)
				printf("Found OTP stream (begin: 0x%04x, raw data len: 0x%04x (%u), type: %u (%s), version: %u)\n",
				       addr, len, len, str->type,
				       strdesc ? strdesc->name : "unknown",
				       str->version);

			if (strdesc && strdesc->proc)
				strdesc->proc(aem, str->data,
					      len - sizeof(*str));

			strcode = 0xff;	/* Indicate outside stream state */
		}
	}

	/**
	 * OTP does not contain a checksum correction, so update unpacked
	 * caldata checksum manually.
	 */
	eep = (void *)aem->unpacked_buf;
	eep->baseEepHeader.checksum = 0xffff;
	eep->baseEepHeader.checksum =
		eep_calc_csum((uint16_t *)aem->unpacked_buf,
			      sizeof(*eep) / sizeof(uint16_t));

	aem->eep_len = QCA9880_OTP_SIZE / sizeof(uint16_t);
	aem->unpacked_len = sizeof(struct qca9880_eeprom);
	memcpy(&emp->eep, aem->unpacked_buf, sizeof(emp->eep));

exit:
	OTP_DISABLE();

	return aem->eep_len != 0;
}

static int eep_9880_check(struct atheepmgr *aem)
{
	struct eep_9880_priv *emp = aem->eepmap_priv;
	struct qca9880_eeprom *eep = &emp->eep;
	struct qca9880_base_eep_hdr *pBase = &eep->baseEepHeader;
	uint16_t sum;
	int i;

	if (pBase->length != sizeof(*eep) &&
	    bswap_16(pBase->length) != sizeof(*eep)) {
		fprintf(stderr, "Bad EEPROM length 0x%04x/0x%04x (expect 0x%04x)\n",
			pBase->length, bswap_16(pBase->length),
			(unsigned int)sizeof(*eep));
		return false;
	}

	/**
	 * Data could come in a compressed form, so calc checksum on the private
	 * copy of the decomressed calibration data *before* endians fix.
	 *
	 * NB: take pointer another one time from container to avoid warning
	 * about a *possible* unaligned access
	 */
	sum = eep_calc_csum((uint16_t *)&emp->eep,
			    sizeof(emp->eep) / sizeof(uint16_t));
	if (sum != 0xffff) {
		fprintf(stderr, "Bad EEPROM checksum 0x%04x\n", sum);
		return false;
	}

	if (!!(pBase->opCapBrdFlags.miscFlags & AR5416_EEPMISC_BIG_ENDIAN) !=
	    aem->host_is_be) {
		struct qca9880_modal_eep_hdr *pModal;

		printf("EEPROM Endianness is not native.. Changing.\n");

		bswap_16_inplace(pBase->length);
		bswap_16_inplace(pBase->checksum);
		bswap_16_inplace(pBase->regDmn[0]);
		bswap_16_inplace(pBase->regDmn[1]);
		bswap_16_inplace(pBase->binBuildNumber);

		pModal = &eep->modalHeader5G;
		bswap_32_inplace(pModal->antCtrlCommon);
		bswap_32_inplace(pModal->antCtrlCommon2);
		for (i = 0; i < ARRAY_SIZE(pModal->antCtrlChain); ++i)
			bswap_16_inplace(pModal->antCtrlChain[i]);

		pModal = &eep->modalHeader2G;
		bswap_32_inplace(pModal->antCtrlCommon);
		bswap_32_inplace(pModal->antCtrlCommon2);
		for (i = 0; i < ARRAY_SIZE(pModal->antCtrlChain); ++i)
			bswap_16_inplace(pModal->antCtrlChain[i]);
	}

	return true;
}

static void eep_9880_dump_base_header(struct atheepmgr *aem)
{
	const struct eep_9880_priv *emp = aem->eepmap_priv;
	const struct qca9880_eeprom *eep = &emp->eep;
	const struct qca9880_base_eep_hdr *pBase = &eep->baseEepHeader;

	EEP_PRINT_SECT_NAME("EEPROM Base Header");

	printf("%-30s : 0x%04X\n", "Length", pBase->length);
	printf("%-30s : 0x%04X\n", "Checksum", pBase->checksum);
	printf("%-30s : %d\n", "EEP Version", pBase->eepromVersion);
	printf("%-30s : %d\n", "Template Version", pBase->templateVersion);
	printf("%-30s : %02X:%02X:%02X:%02X:%02X:%02X\n",
	       "MacAddress",
	       pBase->macAddr[0], pBase->macAddr[1], pBase->macAddr[2],
	       pBase->macAddr[3], pBase->macAddr[4], pBase->macAddr[5]);
	printf("%-30s : 0x%04X\n", "RegDomain1", pBase->regDmn[0]);
	printf("%-30s : 0x%04X\n", "RegDomain2", pBase->regDmn[1]);

	printf("%-30s : %d\n", "Allow 5GHz",
	       !!(pBase->opCapBrdFlags.opFlags & QCA9880_OPFLAGS_11A));
	printf("%-30s : %d/%d\n", "Allow 5GHz HT20/HT40",
	       !!(pBase->opCapBrdFlags.opFlags & QCA9880_OPFLAGS_5G_HT20),
	       !!(pBase->opCapBrdFlags.opFlags & QCA9880_OPFLAGS_5G_HT40));
	printf("%-30s : %d/%d/%d\n", "Allow 5GHz VHT20/VHT40/VHT80",
	       !!(pBase->opCapBrdFlags.opFlags2 & QCA9880_OPFLAGS2_5G_VHT20),
	       !!(pBase->opCapBrdFlags.opFlags2 & QCA9880_OPFLAGS2_5G_VHT40),
	       !!(pBase->opCapBrdFlags.opFlags2 & QCA9880_OPFLAGS2_5G_VHT80));
	printf("%-30s : %d\n", "Allow 2GHz",
	       !!(pBase->opCapBrdFlags.opFlags & QCA9880_OPFLAGS_11G));
	printf("%-30s : %d/%d\n", "Allow 2GHz HT20/HT40",
	       !!(pBase->opCapBrdFlags.opFlags & QCA9880_OPFLAGS_2G_HT20),
	       !!(pBase->opCapBrdFlags.opFlags & QCA9880_OPFLAGS_2G_HT40));
	printf("%-30s : %d/%d\n", "Allow 2GHz VHT20/VHT40",
	       !!(pBase->opCapBrdFlags.opFlags2 & QCA9880_OPFLAGS2_2G_VHT20),
	       !!(pBase->opCapBrdFlags.opFlags2 & QCA9880_OPFLAGS2_2G_VHT40));

	printf("%-30s : 0x%04X\n", "Cal Bin Ver", pBase->binBuildNumber);
	printf("%-30s : 0x%02X\n", "TX Mask", pBase->txrxMask >> 4);
	printf("%-30s : 0x%02X\n", "RX Mask", pBase->txrxMask & 0x0f);
	printf("%-30s : %d\n", "Tx Gain", pBase->txrxgain >> 4);
	printf("%-30s : %d\n", "Rx Gain", pBase->txrxgain & 0xf);
	printf("%-30s : %d\n", "Power Table Offset", pBase->pwrTableOffset);
	printf("%-30s : %d\n", "CCK/OFDM Pwr Delta, dB", pBase->deltaCck20);
	printf("%-30s : %d\n", "40/20 Pwr Delta, dB", pBase->delta4020);
	printf("%-30s : %d\n", "80/20 Pwr Delta, dB", pBase->delta8020);

	printf("\nCustomer Data in hex:\n");
	hexdump_print(pBase->custData, sizeof(pBase->custData));

	printf("\n");
}

static void eep_9880_dump_modal_header(struct atheepmgr *aem)
{
#define PR_LINE(_token, _cb, ...)				\
	do {							\
		printf("%-33s :", _token);			\
		if (opFlags & QCA9880_OPFLAGS_11G) {		\
			_cb(eep->modalHeader2G, ## __VA_ARGS__);\
			printf("  %-20s", buf);			\
		}						\
		if (opFlags & QCA9880_OPFLAGS_11A) {		\
			_cb(eep->modalHeader5G, ## __VA_ARGS__);\
			printf("  %s", buf);			\
		}						\
		printf("\n");					\
	} while (0);
#define __PR_FMT_CONV(_fmt, _val, _conv)			\
		snprintf(buf, sizeof(buf), _fmt, _conv(_val))
#define __PR_FMT(_fmt, _val)	__PR_FMT_CONV(_fmt, _val, )
#define _PR_CB_DEC(_hdr, _field)				\
		__PR_FMT("%d", _hdr._field)
#define _PR_CB_HEX(_hdr, _field)				\
		__PR_FMT("0x%X", _hdr._field)
#define PR_DEC(_token, _field)					\
		PR_LINE(_token, _PR_CB_DEC, _field)
#define PR_HEX(_token, _field)					\
		PR_LINE(_token, _PR_CB_HEX, _field)

	const struct eep_9880_priv *emp = aem->eepmap_priv;
	const struct qca9880_eeprom *eep = &emp->eep;
	const struct qca9880_base_eep_hdr *pBase = &eep->baseEepHeader;
	const uint8_t opFlags = pBase->opCapBrdFlags.opFlags;
	char buf[0x20];

	EEP_PRINT_SECT_NAME("EEPROM Modal Header");

	printf("%35s", "");
	if (opFlags & QCA9880_OPFLAGS_11G)
		printf("  %-20s", "2G");
	if (opFlags & QCA9880_OPFLAGS_11A)
		printf("  %s", "5G");
	printf("\n\n");

	PR_HEX("Antenna Ctrl Chain 0", antCtrlChain[0]);
	PR_HEX("Antenna Ctrl Chain 1", antCtrlChain[1]);
	PR_HEX("Antenna Ctrl Chain 2", antCtrlChain[2]);
	PR_HEX("Antenna Ctrl Common", antCtrlCommon);
	PR_HEX("Antenna Ctrl Common2", antCtrlCommon2);
	PR_DEC("Antenna Gain", antennaGain);
	PR_DEC("NF Thresh", noiseFloorThresh);

	printf("\n");

#undef PR_HEX
#undef PR_DEC
#undef _PR_CB_HEX
#undef _PR_CB_DEC
#undef __PR_FMT
#undef __PR_FMT_CONV
#undef PR_LINE
}

static void eep_9880_dump_tgt_pow_legacy(const uint8_t *freqs, int nfreqs,
					 const struct qca9880_cal_tgt_pow_legacy *data,
					 const char * const rates[], int is_2g)
{
#define MARGIN		"    "
	int i, j;

	printf(MARGIN "%18s, MHz:", "Freq");
	for (j = 0; j < nfreqs; ++j)
		printf("  %4u", FBIN2FREQ(freqs[j], is_2g));
	printf("\n");
	printf(MARGIN "------------------------");
	for (j = 0; j < nfreqs; ++j)
		printf("  ----");
	printf("\n");

	for (i = 0; i < 4; ++i) {
		printf(MARGIN "%18s, dBm:", rates[i]);
		for (j = 0; j < nfreqs; ++j)
			printf("  %4.1f", (double)data[j].tPow2x[i] / 2);
		printf("\n");
	}
}

static void eep_9880_dump_tgt_pow_vht(const uint8_t *freqs, int nfreqs,
				      const struct qca9880_cal_tgt_pow_vht *data,
				      const uint8_t *ext_delta, int bwidx,
				      int maxstreams, int is_2g)
{
#define MARGIN		"    "
	static const struct {
		const char * const ht_mcs;
		const char * const vht_mcs;
		int nstreams;
		int rate_idx;
	} rates[24] = {
		{"    0", "    0", 1,  0}, {"  1-2", "  1-2", 1,  1},
		{"  3-4", "  3-4", 1,  2}, {"    5", "    5", 1,  3},
		{"    6", "    6", 1,  4}, {"    7", "    7", 1,  5},
		{"     ", "    8", 1,  6}, {"     ", "    9", 1,  7},
		{"    8", "   10", 2,  0}, {" 9-10", "11-12", 2,  1},
		{"11-12", "13-14", 2,  2}, {"   13", "   15", 2,  8},
		{"   14", "   16", 2,  9}, {"   15", "   17", 2, 10},
		{"     ", "   18", 2, 11}, {"     ", "   19", 2, 12},
		{"   16", "   20", 3,  0}, {"17-18", "21-22", 3,  1},
		{"19-20", "23-24", 3,  2}, {"   21", "   25", 3, 13},
		{"   22", "   26", 3, 14}, {"   23", "   27", 3, 15},
		{"     ", "   28", 3, 16}, {"     ", "   29", 3, 17},
	};
	int i, j;

	printf(MARGIN " HT MCS VHT MCS | %s, MHz:", "Freq");
	for (j = 0; j < nfreqs; ++j)
		printf("  %4u", FBIN2FREQ(freqs[j], is_2g));
	printf("\n");
	printf(MARGIN "----------------------------");
	for (j = 0; j < nfreqs; ++j)
		printf("  ----");
	printf("\n");

	for (i = 0; i < ARRAY_SIZE(rates); ++i) {
		int sidx = rates[i].nstreams - 1;

		if (rates[i].nstreams > maxstreams)
			break;

		printf(MARGIN "%7s %7s,        dBm:", rates[i].ht_mcs,
		       rates[i].vht_mcs);
		for (j = 0; j < nfreqs; ++j) {
			int ridx = rates[i].rate_idx;
			uint8_t base = data[j].tPow2xBase[sidx];
			uint8_t delta = data[j].tPow2xDelta[ridx / 2];
			int ebidx = (bwidx * nfreqs + j) *
				    QCA9880_TGTPWR_VHT_NUM_RATES + ridx;
			uint8_t ed = (ext_delta[ebidx / 8] >> (ebidx % 8));

			delta = (delta >> (4 * (ridx % 2))) & 0x0f;
			delta |= (ed << 4) & 0x10;
			printf("  %4.1f", (double)(base + delta) / 2);
		}
		printf("\n");
	}
}

static const char * const eep_9880_rates_cck[4] = {
	"1-5 mbps (L)", "5 mbps (S)", "11 mbps (L)", "11 mbps (S)"
};

static const char * const eep_9880_rates_ofdm[4] = {
	"6-24 mbps", "36 mbps", "48 mbps", "54 mbps"
};

static void eep_9880_dump_power_info(struct atheepmgr *aem)
{
#define PR_TGT_POW_LEGACY(__pref, __mod, __rates, __is_2g)		\
	do {								\
		EEP_PRINT_SUBSECT_NAME(__pref " per-rate target power");\
		eep_9880_dump_tgt_pow_legacy(eep->targetFreqbin ## __mod,\
					     ARRAY_SIZE(eep->targetFreqbin ## __mod),\
					     eep->targetPower ## __mod,	\
					     __rates, __is_2g);		\
		printf("\n");						\
	} while (0);
#define PR_TGT_POW_VHT(__pref, __mod, __is_2g)				\
	do {								\
		EEP_PRINT_SUBSECT_NAME(__pref " per-rate target power");\
		eep_9880_dump_tgt_pow_vht(eep->targetFreqbin ## __mod,\
					  ARRAY_SIZE(eep->targetFreqbin ## __mod),\
					  eep->targetPower ## __mod,	\
					  __is_2g ? eep->extTPow2xDelta2G : \
					  eep->extTPow2xDelta5G,	\
					  QCA9880_TGTPWR_VHT_ ## __mod ## _BWIDX,\
					  maxstreams, __is_2g);		\
		printf("\n");						\
	} while (0);
#define PR_CTL(__pref, __band, __is_2g)					\
	do {								\
		EEP_PRINT_SUBSECT_NAME(__pref " CTL data");		\
		ar9300_dump_ctl(eep->ctlIndex ## __band,		\
				(uint8_t *)eep->ctlFreqBin ## __band,	\
				(uint8_t *)eep->ctlData ## __band,	\
				QCA9880_NUM_ ## __band ## _CTLS,	\
				QCA9880_NUM_ ## __band ## _BAND_EDGES,	\
				__is_2g);				\
	} while (0);

	static const int mask2maxstreams[] = {0, 1, 1, 2, 1, 2, 2, 3};
	const struct eep_9880_priv *emp = aem->eepmap_priv;
	const struct qca9880_eeprom *eep = &emp->eep;
	int txmask, maxstreams;

	EEP_PRINT_SECT_NAME("EEPROM Power Info");

	txmask = eep->baseEepHeader.txrxMask >> 4;
	if (txmask >= ARRAY_SIZE(mask2maxstreams)) {
		printf("Invalid TxMask value -- 0x%04x, use maximum possible value 0x7\n\n",
		       txmask);
		txmask = 0x7;
	}
	maxstreams = mask2maxstreams[txmask];

	if (eep->baseEepHeader.opCapBrdFlags.opFlags & QCA9880_OPFLAGS_11G) {
		PR_TGT_POW_LEGACY("2 GHz CCK", 2GCck, eep_9880_rates_cck, 1);
		PR_TGT_POW_LEGACY("2 GHz OFDM", 2GLeg, eep_9880_rates_ofdm, 1);
		PR_TGT_POW_VHT("2 GHz HT20", 2GVHT20, 1);
		PR_TGT_POW_VHT("2 GHz HT40", 2GVHT40, 1);
	}

	if (eep->baseEepHeader.opCapBrdFlags.opFlags & QCA9880_OPFLAGS_11A) {
		PR_TGT_POW_LEGACY("5 GHz OFDM", 5GLeg, eep_9880_rates_ofdm, 0);
		PR_TGT_POW_VHT("5 GHz HT20/VHT20", 5GVHT20, 0);
		PR_TGT_POW_VHT("5 GHz HT40/VHT40", 5GVHT40, 0);
		PR_TGT_POW_VHT("5 GHz VHT80", 5GVHT80, 0);
	}

	if (eep->baseEepHeader.opCapBrdFlags.opFlags & QCA9880_OPFLAGS_11G)
		PR_CTL("2 GHz", 2G, 1);
	if (eep->baseEepHeader.opCapBrdFlags.opFlags & QCA9880_OPFLAGS_11A)
		PR_CTL("5 GHz", 5G, 0);

#undef PR_CTL
#undef PR_TGT_POW_VHT
#undef PR_TGT_POW_LEGACY
}

const struct eepmap eepmap_9880 = {
	.name = "9880",
	.desc = "EEPROM map for earlier .11ac chips (QCA9880/QCA9882/QCA9892/etc.)",
	.chip_regs = {
		.srev = 0x40ec,
	},
	.priv_data_sz = sizeof(struct eep_9880_priv),
	.eep_buf_sz = QCA9880_EEPROM_SIZE / sizeof(uint16_t),
	.unpacked_buf_sz = sizeof(struct qca9880_eeprom),
	.templates = eep_9880_templates,
	.load_blob = eep_9880_load_blob,
	.load_otp = eep_9880_load_otp,
	.check_eeprom = eep_9880_check,
	.dump = {
		[EEP_SECT_BASE] = eep_9880_dump_base_header,
		[EEP_SECT_MODAL] = eep_9880_dump_modal_header,
		[EEP_SECT_POWER] = eep_9880_dump_power_info,
	},
};
