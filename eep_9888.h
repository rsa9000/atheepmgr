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

#ifndef EEP_9888_H
#define EEP_9888_H

#define QCA9888_CUSTOMER_DATA_SIZE		20

struct qca9888_base_eep_hdr {
	uint16_t length;
	uint16_t checksum;
	uint8_t eepromVersion;
	uint8_t templateVersion;
	uint8_t macAddr[6];
	uint8_t __unkn_0c[32];
	uint8_t custData[QCA9888_CUSTOMER_DATA_SIZE];
} __attribute__ ((packed));

struct qca9888_eeprom {
	struct qca9888_base_eep_hdr baseEepHeader;

	uint8_t __unkn_0040[12000]; /* to match structure size to the EEPROM data size */
} __attribute__ ((packed));

/* Structure size watchdog */
_Static_assert(sizeof(struct qca9888_eeprom) == 12064, "Invalid QCA9888 EEPROM structure size");

#endif
