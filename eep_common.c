/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * Copyright (c) 2013,2017-2021 Sergey Ryazanov <ryazanov.s.a@gmail.com>
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
#include "eep_common.h"

const char * const sDeviceType[] = {
	"UNKNOWN [0] ",
	"Cardbus     ",
	"PCI         ",
	"MiniPCI     ",
	"Access Point",
	"PCIExpress  ",
	"UNKNOWN [6] ",
	"UNKNOWN [7] ",
};

const char * const sAccessType[] = {
	"ReadWrite", "WriteOnly", "ReadOnly", "NoAccess"
};

const char * const eep_rates_cck[AR5416_NUM_TARGET_POWER_RATES_LEG] = {
	"1 mbps", "2 mbps", "5.5 mbps", "11 mbps"
};

const char * const eep_rates_ofdm[AR5416_NUM_TARGET_POWER_RATES_LEG] = {
	"6-24 mbps", "36 mbps", "48 mbps", "54 mbps"
};

const char * const eep_rates_ht[AR5416_NUM_TARGET_POWER_RATES_HT] = {
	"MCS 0/8", "MCS 1/9", "MCS 2/10", "MCS 3/11",
	"MCS 4/12", "MCS 5/13", "MCS 6/14", "MCS 7/15"
};

const char * const eep_ctldomains[] = {
	 "Unknown (0)",          "FCC",  "Unknown (2)",         "ETSI",
	         "MKK",  "Unknown (5)",  "Unknown (6)",  "Unknown (7)",
	 "Unknown (8)",  "Unknown (9)", "Unknown (10)", "Unknown (11)",
	"Unknown (12)", "Unknown (13)",    "SD no ctl",       "No ctl"
};

const char * const eep_ctlmodes[] = {
	   "5GHz OFDM",     "2GHz CCK",    "2GHz OFDM",   "5GHz Turbo",
	  "2GHz Turbo",    "2GHz HT20",    "5GHz HT20",    "2GHz HT40",
	   "5GHz HT40",   "5GHz VHT20",   "2GHz VHT20",   "5GHz VHT40",
	  "2GHz VHT40",   "5GHz VHT80", "Unknown (14)", "Unknown (15)"
};

/**
 * Detect possible EEPROM I/O byteswapping and toggle I/O byteswap compensation
 * if need it to consistently load EEPROM data.
 *
 * NB: all offsets are in 16-bits words
 */
bool __ar5416_toggle_byteswap(struct atheepmgr *aem, uint32_t eepmisc_off,
			      uint32_t binbuildnum_off)
{
	bool magic_is_be;
	uint16_t word;

	/* First check whether magic is Little-endian or not */
	if (!EEP_READ(AR5416_EEPROM_MAGIC_OFFSET, &word)) {
		printf("Toggle EEPROM I/O byteswap compensation\n");
		return false;
	}
	magic_is_be = word != AR5416_EEPROM_MAGIC;	/* Constant is LE */

	/**
	 * Now read {opCapFlags,eepMisc} pair of fields that lay in the same
	 * EEPROM word. opCapFlags first bit indicates 5GHz support, while
	 * eepMisc first bit indicates BigEndian EEPROM data. Due to possible
	 * byteswap we are unable to distinguish between these two fields.
	 *
	 * So we have only two certain cases: if first bit in both octets are
	 * set, then we have a Big-endian EEPROM data with activated 5GHz
	 * support, and in opposite if bits in both octets are reset, then we
	 * have a Little-endian EEPROM with disabled 5GHz support. In both cases
	 * I/O byteswap could be detected using only magic field value: as soon
	 * as we know an EEPROM format, magic format mismatch indicates the I/O
	 * byteswap.
	 *
	 * If a first bit set only in one octet, then we are in an abiguity
	 * case:
	 *
	 *  EEP        Host   I/O  word &
	 * type  5GHz  type  swap  0x0101
	 * ------------------------------
	 *  LE     1    LE     N   0x0001
	 *  LE     1    BE     Y   0x0001
	 *  BE     0    LE     Y   0x0001
	 *  BE     0    BE     N   0x0001
	 *  LE     1    LE     Y   0x0100
	 *  LE     1    BE     N   0x0100
	 *  BE     0    LE     N   0x0100
	 *  BE     0    BE     Y   0x0100
	 *
	 *  And we will need some more heuristic to solve it (see below).
	 */
	if (!EEP_READ(eepmisc_off, &word)) {
		fprintf(stderr, "EEPROM misc field read failed\n");
		return false;
	}

	word &= 0x0101;	/* Clear all except 5GHz and BigEndian bits */
	if (word == 0x0000) {/* Clearly not Big-endian EEPROM */
		if (!magic_is_be)
			goto skip_eeprom_io_swap;
		if (aem->verbose > 1)
			printf("Got byteswapped Little-endian EEPROM data\n");
		goto toggle_eeprom_io_swap;
	} else if (word == 0x0101) {/* Clearly Big-endian EEPROM */
		if (magic_is_be)
			goto skip_eeprom_io_swap;
		if (aem->verbose > 1)
			printf("Got byteswapped Big-endian EEPROM data\n");
		goto toggle_eeprom_io_swap;
	}

	if (aem->verbose > 1)
		printf("Data is possibly byteswapped\n");

	/**
	 * Calibration software (ART) version in each seen AR5416/AR92xx EEPROMs
	 * is a 32-bits value that has a following format:
	 *
	 *    .----------- Major version (always zero)
	 *    | .--------- Minor version (always non-zero)
	 *    | | .------- Build (always non-zero)
	 *    | | | .----- Unused (always zero)
	 *    | | | |
	 * 0xMMmmrr00
	 *
	 * For example: 0x00091500 is a cal. software v0.9.21. For Little-endian
	 * EEPROM this version will be saved as 0x00 0x15 0x09 0x00, and for
	 * Big-endian EEPROM this will be saved as 0x00 0x09 0x15 0x00. As you
	 * can see both formats have a common part: first byte is always zero,
	 * while a second one is always non-zero. But if data will be
	 * byteswapped in each 16-bits word, then we will have an opposite
	 * situation: first byte become a non-zero, while the second one become
	 * zero.
	 *
	 * Checking each octet of first 16-bits part of version field is our
	 * endian-agnostic way to detect the byteswapping.
	 */

	if (!EEP_READ(binbuildnum_off, &word)) {
		fprintf(stderr, "Calibration software build read failed\n");
		return false;
	}

	word = le16toh(word);	/* Just to make a byteorder predictable */

	/* First we check for byteswapped case */
	if ((word & 0xff00) == 0 && (word & 0x00ff) != 0)
		goto toggle_eeprom_io_swap;

	/* Now check for non-byteswapped case */
	if ((word & 0xff00) != 0 && (word & 0x00ff) == 0) {
		if (aem->verbose > 1)
			printf("Looks like there are no byteswapping\n");
		goto skip_eeprom_io_swap;
	}

	/* We have some weird software version, giving up */
	if (aem->verbose > 1)
		printf("Unable to detect byteswap, giving up\n");

	if (!magic_is_be)	/* Prefer the Little-endian format */
		goto skip_eeprom_io_swap;

toggle_eeprom_io_swap:
	if (aem->verbose)
		printf("Toggle EEPROM I/O byteswap compensation\n");
	aem->eep_io_swap = !aem->eep_io_swap;

skip_eeprom_io_swap:
	return true;
}

/**
 * NB: size is in 16-bits words
 */
void ar5416_dump_eep_init(const struct ar5416_eep_init *ini, size_t size)
{
	int i, maxregsnum;

	printf("%-20s : 0x%04X\n", "Magic", ini->magic);
	for (i = 0; i < 8; ++i)
		printf("Region%d access       : %s\n", i,
		       sAccessType[(ini->prot >> (i * 2)) & 0x3]);
	printf("%-20s : 0x%04X\n", "Regs init data ptr", ini->iptr);
	printf("\n");

	EEP_PRINT_SUBSECT_NAME("Register(s) initialization data");

	maxregsnum = (2 * size - offsetof(typeof(*ini), regs)) /
		     sizeof(ini->regs[0]);
	for (i = 0; i < maxregsnum; ++i) {
		if (ini->regs[i].addr == 0xffff)
			break;
		printf("  %04X: %04X%04X\n", ini->regs[i].addr,
		       ini->regs[i].val_high, ini->regs[i].val_low);
	}

	printf("\n");
}

static void
ar5416_dump_pwrctl_closeloop_item(const uint8_t *pwr, const uint8_t *vpd,
				  int maxicepts, int maxstoredgains,
				  int gainmask, int power_table_offset)
{
	const char * const gains[AR5416_NUM_PD_GAINS] = {"4", "2", "1", "0.5"};
	uint8_t mpwr[maxicepts * maxstoredgains];
	uint8_t mvpd[ARRAY_SIZE(mpwr) * maxstoredgains];
	/* Map of Mask Gain bit Index to Calibrated per-Gain icepts set Index */
	int mgi2cgi[ARRAY_SIZE(gains)];
	int cgii[maxstoredgains];	/* Array of indexes for merge */
	int gainidx, ngains, pwridx, npwr;	/* Indexes and index limits */
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
	for (gainidx = 0; gainidx < ARRAY_SIZE(gains); ++gainidx) {
		if (gainmask & (1 << gainidx)) {
			mgi2cgi[gainidx] = ngains;
			ngains++;
		} else {
			mgi2cgi[gainidx] = -1;
		}
	}
	if (ngains > maxstoredgains) {
		printf("      PD gain mask activates more gains then possible to store -- %d > %d\n",
		       ngains, maxstoredgains);
		return;
	}

	/* Merge calibration per-gain power lists to filter duplicates */
	memset(mpwr, 0xff, sizeof(mpwr));
	memset(mvpd, 0xff, sizeof(mvpd));
	memset(cgii, 0x00, sizeof(cgii));
	for (pwridx = 0; pwridx < ARRAY_SIZE(mpwr); ++pwridx) {
		pwrmin = 0xff;
		/* Looking for unmerged yet power value */
		for (gainidx = 0; gainidx < ngains; ++gainidx) {
			if (cgii[gainidx] >= maxicepts)
				continue;
			if (pwr[gainidx * maxicepts + cgii[gainidx]] < pwrmin)
				pwrmin = pwr[gainidx * maxicepts + cgii[gainidx]];
		}
		if (pwrmin == 0xff)
			break;
		mpwr[pwridx] = pwrmin;
		/* Copy Vpd of all gains for this power */
		for (gainidx = 0; gainidx < ngains; ++gainidx) {
			if (cgii[gainidx] >= AR5416_PD_GAIN_ICEPTS ||
			    pwr[gainidx * maxicepts + cgii[gainidx]] != pwrmin)
				continue;
			mvpd[pwridx * maxstoredgains + gainidx] =
				vpd[gainidx * maxicepts + cgii[gainidx]];
			cgii[gainidx]++;
		}
	}
	npwr = pwridx;

	/* Print merged data */
	printf("      Tx Power, dBm:");
	for (pwridx = 0; pwridx < npwr; ++pwridx)
		printf(" %5.2f", (double)mpwr[pwridx] / 4 +
				 power_table_offset);
	printf("\n");
	printf("      --------------");
	for (pwridx = 0; pwridx < npwr; ++pwridx)
		printf(" -----");
	printf("\n");
	for (gainidx = 0; gainidx < ARRAY_SIZE(gains); ++gainidx) {
		if (!(gainmask & (1 << gainidx)))
			continue;
		printf("      Gain x%-3s VPD:", gains[gainidx]);
		for (pwridx = 0; pwridx < npwr; ++pwridx) {
			uint8_t vpd = mvpd[pwridx * maxstoredgains + mgi2cgi[gainidx]];

			if (vpd == 0xff)
				printf("      ");
			else
				printf("   %3u", vpd);
		}
		printf("\n");
	}
}

/**
 * Data is an array of per-chain & per-frequency sets of calibrations.
 * Each set calibrations consists of two parts: first part contains a set of
 * output power values, while the second part contains a corresponding power
 * detector values. Each part (power and detector) has a similar structure, it
 * is an array of per PD gain sets of measurements (icepts). Each type of
 * values (power and detector) has the similar size of one octet.
 *
 * So to calculate position of per-chain & per-frequency data we should know
 * size of this data. Such data block size is a sum of its parts, i.e. sum of
 * output power data size and power detector data size. The size of each part
 * is a multiplication of a number of PD gains (maxstoredgains) and of a number
 * of calibration points (maxicepts).
 *
 * So having all this numbers we are able to easly calculate size of various
 * elements and their positions.
 */
void ar5416_dump_pwrctl_closeloop(const uint8_t *freqs, int maxfreqs,
				  bool is_2g, int maxchains, int chainmask,
				  const void *data, int maxicepts,
				  int maxstoredgains, int gainmask,
				  int power_table_offset)
{
	/* Sizes of TxPower and Detector sets of data */
	const int fpwrdatasz = maxicepts * maxstoredgains * sizeof(uint8_t);
	const int fvpddatasz = maxicepts * maxstoredgains * sizeof(uint8_t);
	const int fdatasz = fpwrdatasz + fvpddatasz;	/* Per-chain & per-freq data sz */
	const uint8_t *fdata, *fpwrdata, *fvpddata;
	int chain, freq;	/* Indexes */

	for (chain = 0; chain < maxchains; ++chain) {
		if (!(chainmask & (1 << chain)))
			continue;
		printf("  Chain %d:\n", chain);
		printf("\n");
		for (freq = 0; freq < maxfreqs; ++freq) {
			if (freqs[freq] == AR5416_BCHAN_UNUSED)
				break;

			printf("    %4u MHz:\n", FBIN2FREQ(freqs[freq], is_2g));

			fdata = data + fdatasz * (chain * maxfreqs + freq);
			fpwrdata = fdata + 0;/* Power data begins immediatly */
			fvpddata = fdata + fpwrdatasz;	/* Skip power data */

			ar5416_dump_pwrctl_closeloop_item(fpwrdata, fvpddata,
							  maxicepts,
							  maxstoredgains,
							  gainmask,
							  power_table_offset);

			printf("\n");
		}
	}
}

void ar5416_dump_target_power(const struct ar5416_cal_target_power *caldata,
			      int maxchans, const char * const rates[],
			      int nrates, bool is_2g)
{
#define MARGIN		"    "
#define TP_ITEM_SIZE	(sizeof(struct ar5416_cal_target_power) + \
			 nrates * sizeof(uint8_t))
#define TP_NEXT_CHAN(__tp)	((void *)((uint8_t *)(__tp) + TP_ITEM_SIZE))

	const struct ar5416_cal_target_power *tp;
	int nchans, i, j;

	printf(MARGIN "%10s, MHz:", "Freq");
	tp = caldata;
	for (j = 0; j < maxchans; ++j, tp = TP_NEXT_CHAN(tp)) {
		if (tp->bChannel == AR5416_BCHAN_UNUSED)
			break;
		printf("  %4u", FBIN2FREQ(tp->bChannel, is_2g));
	}
	nchans = j;
	printf("\n");
	printf(MARGIN "----------------");
	for (j = 0; j < nchans; ++j)
		printf("  ----");
	printf("\n");

	for (i = 0; i < nrates; ++i) {
		printf(MARGIN "%10s, dBm:", rates[i]);
		tp = caldata;
		for (j = 0; j < nchans; ++j, tp = TP_NEXT_CHAN(tp))
			printf("  %4.1f", (double)tp->tPow2x[i] / 2);
		printf("\n");
	}

#undef TP_NEXT_CHAN
#undef TP_ITEM_SIZE
#undef MARGIN
}

void ar5416_dump_ctl_edges(const struct ar5416_cal_ctl_edges *edges,
			   int maxradios, int maxedges, bool is_2g)
{
	const struct ar5416_cal_ctl_edges *e;
	int edge, rnum;
	bool open;

	for (rnum = 0; rnum < maxradios; ++rnum) {
		printf("\n");
		if (maxradios > 1)
			printf("    %d radio(s) Tx:\n", rnum + 1);
		printf("           Edges, MHz:");
		for (edge = 0, open = true; edge < maxedges; ++edge) {
			e = &edges[rnum * maxedges + edge];
			if (!e->bChannel)
				break;
			printf(" %c%4u%c",
			       !CTL_EDGE_FLAGS(e->ctl) && open ? '[' : ' ',
			       FBIN2FREQ(e->bChannel, is_2g),
			       !CTL_EDGE_FLAGS(e->ctl) && !open ? ']' : ' ');
			if (!CTL_EDGE_FLAGS(e->ctl))
				open = !open;
		}
		printf("\n");
		printf("      MaxTxPower, dBm:");
		for (edge = 0; edge < maxedges; ++edge) {
			e = &edges[rnum * maxedges + edge];
			if (!e->bChannel)
				break;
			printf("  %4.1f ", (double)CTL_EDGE_POWER(e->ctl) / 2);
		}
		printf("\n");
	}
}

void ar5416_dump_ctl(const uint8_t *index,
		     const struct ar5416_cal_ctl_edges *data,
		     int maxctl, int maxchains, int maxradios, int maxedges)
{
	int i;
	uint8_t ctl;

	for (i = 0; i < maxctl; ++i) {
		if (!index[i])
			break;
		ctl = index[i];
		printf("  %s %s:\n", eep_ctldomains[ctl >> 4],
		       eep_ctlmodes[ctl & 0x0f]);

		ar5416_dump_ctl_edges(data + i * (maxchains * maxedges),
				      maxradios, maxedges,
				      eep_ctlmodes[ctl & 0x0f][0] == '2'/*:)*/);

		printf("\n");
	}
}

void ar9300_comp_hdr_unpack(const uint8_t *p, struct ar9300_comp_hdr *hdr)
{
	unsigned long value[4] = {p[0], p[1], p[2], p[3]};

	hdr->comp = (value[0] >> 5) & 0x0007;
	hdr->ref = (value[0] & 0x001f) | ((value[1] >> 2) & 0x0020);
	hdr->len = ((value[1] << 4) & 0x07f0) | ((value[2] >> 4) & 0x000f);
	hdr->maj = value[2] & 0x000f;
	hdr->min = value[3] & 0x00ff;
}

uint16_t ar9300_comp_cksum(const uint8_t *data, int dsize)
{
	int it, checksum = 0;

	for (it = 0; it < dsize; it++) {
		checksum += data[it];
		checksum &= 0xffff;
	}

	return checksum;
}

static bool ar9300_uncompress_block(struct atheepmgr *aem, uint8_t *out,
				    int out_size, const uint8_t *in, int in_len)
{
	int it;
	int spot;
	int offset;
	int length;

	spot = 0;

	for (it = 0; it < in_len; it += length + 2) {
		offset = in[it];
		offset &= 0xff;
		spot += offset;
		length = in[it + 1];
		length &= 0xff;

		if (length > 0 && spot >= 0 && spot+length <= out_size) {
			if (aem->verbose)
				printf("Restore at %d: spot=%d offset=%d length=%d\n",
				       it, spot, offset, length);
			memcpy(&out[spot], &in[it+2], length);
			spot += length;
		} else if (length > 0) {
			fprintf(stderr,
				"Bad restore at %d: spot=%d offset=%d length=%d\n",
				it, spot, offset, length);
			return false;
		}
	}

	return true;
}

int ar9300_compress_decision(struct atheepmgr *aem, int it,
			     struct ar9300_comp_hdr *hdr, uint8_t *out,
			     const uint8_t *data, int out_size, int *pcurrref,
			     const uint8_t *(*tpl_lookup_cb)(int))
{
	bool res;

	switch (hdr->comp) {
	case AR9300_COMP_NONE:
		if (hdr->len != out_size) {
			fprintf(stderr,
				"EEPROM structure size mismatch memory=%d eeprom=%d\n",
				out_size, hdr->len);
			return -1;
		}
		memcpy(out, data, hdr->len);
		if (aem->verbose)
			printf("restored eeprom %d: uncompressed, length %d\n",
			       it, hdr->len);
		break;

	case AR9300_COMP_BLOCK:
		if (hdr->ref != *pcurrref) {
			const uint8_t *tpl;

			tpl = tpl_lookup_cb(hdr->ref);
			if (tpl == NULL) {
				fprintf(stderr,
					"can't find reference eeprom struct %d\n",
					hdr->ref);
				return -1;
			}
			memcpy(out, tpl, out_size);
			*pcurrref = hdr->ref;
		}
		if (aem->verbose)
			printf("Restore eeprom %d: block, reference %d, length %d\n",
			       it, hdr->ref, hdr->len);
		res = ar9300_uncompress_block(aem, out, out_size,
					      data, hdr->len);
		if (!res)
			return -1;
		break;

	default:
		fprintf(stderr, "unknown compression code %d\n", hdr->comp);
		return -1;
	}

	return 0;
}

static void ar9300_dump_ctl_edges(const uint8_t *freqs, const uint8_t *data,
				  int maxedges, bool is_2g)
{
	bool open;
	int i;

	printf("           Edges, MHz:");
	for (i = 0, open = true; i < maxedges; ++i) {
		if (freqs[i] == 0xff || freqs[i] == 0x00)
			continue;
		printf(" %c%4u%c",
		       !CTL_EDGE_FLAGS(data[i]) && open ? '[' : ' ',
		       FBIN2FREQ(freqs[i], is_2g),
		       !CTL_EDGE_FLAGS(data[i]) && !open ? ']' : ' ');
		if (!CTL_EDGE_FLAGS(data[i]))
			open = !open;
	}
	printf("\n");
	printf("      MaxTxPower, dBm:");
	for (i = 0, open = true; i < maxedges; ++i) {
		if (freqs[i] == 0xff || freqs[i] == 0x00)
			continue;
		printf("  %4.1f ", (double)CTL_EDGE_POWER(data[i]) / 2);
	}
	printf("\n");
}

void ar9300_dump_ctl(const uint8_t *index, const uint8_t *freqs,
		     const uint8_t *data, int maxctl, int maxedges, bool is_2g)
{
	uint8_t ctl;
	int i;

	for (i = 0; i < maxctl; ++i) {
		ctl = index[i];
		if (ctl == 0xff || ctl == 0x00)
			continue;
		printf("  %s %s:\n", eep_ctldomains[ctl >> 4],
		       eep_ctlmodes[ctl & 0x0f]);

		ar9300_dump_ctl_edges(freqs + maxedges * i,
				      data + maxedges * i,
				      maxedges, is_2g);

		printf("\n");
	}
}

uint16_t eep_calc_csum(const uint16_t *buf, size_t len)
{
	uint16_t csum = 0;
	size_t i;

	for (i = 0; i < len; i++)
		csum ^= *buf++;

	return csum;
}
