/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
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

#include "edump.h"

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
	   "5GHz OFDM",     "2GHz CCK",    "2GHz OFDM",  "Unknown (3)",
	 "Unknown (4)",    "2GHz HT20",    "5GHz HT20",    "2GHz HT40",
	   "5GHz HT40",  "Unknown (9)", "Unknown (10)", "Unknown (11)",
	"Unknown (12)", "Unknown (13)", "Unknown (14)", "Unknown (15)"
};

void ar5416_dump_target_power(const struct ar5416_cal_target_power *caldata,
			      int maxchans, const char * const rates[],
			      int nrates, int is_2g)
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
			   int maxradios, int maxedges, int is_2g)
{
	const struct ar5416_cal_ctl_edges *e;
	int edge, rnum, open;

	for (rnum = 0; rnum < maxradios; ++rnum) {
		printf("\n");
		if (maxradios > 1)
			printf("    %d radio(s) Tx:\n", rnum + 1);
		printf("           Edges, MHz:");
		for (edge = 0, open = 1; edge < maxedges; ++edge) {
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

uint16_t eep_calc_csum(const uint16_t *buf, size_t len)
{
	uint16_t csum = 0;
	size_t i;

	for (i = 0; i < len; i++)
		csum ^= *buf++;

	return csum;
}
