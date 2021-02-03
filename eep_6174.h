/*
 * Copyright (c) 2021 Sergey Ryazanov <ryazanov.s.a@gmail.com>
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

#ifndef EEP_6174_H
#define EEP_6174_H

#define QCA6174_CUSTOMER_DATA_SIZE		20

#define QCA6174_NUM_2G_CTLS			18
#define QCA6174_NUM_2G_BAND_EDGES		4
#define QCA6174_NUM_5G_CTLS			18
#define QCA6174_NUM_5G_BAND_EDGES		8

struct qca6174_base_eep_hdr {
	uint16_t length;
	uint16_t checksum;
	uint8_t eepromVersion;
	uint8_t templateVersion;
	uint8_t macAddr[6];
	uint8_t __unkn_0c[32];
	uint8_t custData[QCA6174_CUSTOMER_DATA_SIZE];
} __attribute__ ((packed));

struct qca6174_eeprom {
	struct qca6174_base_eep_hdr baseEepHeader;

	uint8_t __unkn_0040[2008];

	uint8_t ctlIndex2G[QCA6174_NUM_2G_CTLS];
	uint8_t __pad_082a[2];
	uint8_t ctlFreqBin2G[QCA6174_NUM_2G_CTLS][QCA6174_NUM_2G_BAND_EDGES];
	uint8_t ctlData2G[QCA6174_NUM_2G_CTLS][QCA6174_NUM_2G_BAND_EDGES];

	uint8_t __unkn_08bc[3766];

	uint8_t ctlIndex5G[QCA6174_NUM_5G_CTLS];
	uint8_t __pad_1784[4];
	uint8_t ctlFreqBin5G[QCA6174_NUM_5G_CTLS][QCA6174_NUM_5G_BAND_EDGES];
	uint8_t ctlData5G[QCA6174_NUM_5G_CTLS][QCA6174_NUM_5G_BAND_EDGES];

	uint8_t __unkn_18a8[1812]; /* to match structure size to the EEPROM data size */
} __attribute__ ((packed));

/* Structure size watchdog */
_Static_assert(sizeof(struct qca6174_eeprom) == 8124, "Invalid QCA6174 EEPROM structure size");

#endif
