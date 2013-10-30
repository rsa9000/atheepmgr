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

/* This max value is almost arbitrary and it selection is based on my
 * own observations. */
#define AR5211_SIZE_MAX			0x2000		/* 16KB EEPROM */
#define AR5211_SIZE_DEF			0x0400		/* 2KB EEPROM */

#define AR5211_EEP_ENDLOC_LO		0x001b		/* Lower word */

#define AR5211_EEP_ENDLOC_UP		0x001c		/* Upper word */
#define AR5211_EEP_ENDLOC_SIZE		0x000f		/* EEPROM size */
#define AR5211_EEP_ENDLOC_SIZE_S	0
#define AR5211_EEP_ENDLOC_LOC		0xfff0		/* End location */
#define AR5211_EEP_ENDLOC_LOC_S		4

#endif	/* EEP_5211_H */
