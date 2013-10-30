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

static bool eep_5211_fill(struct atheepmgr *aem)
{
	uint16_t endloc_up, endloc_lo;
	int len = 0, addr;
	uint16_t *buf = aem->eep_buf;

	if (!EEP_READ(AR5211_EEP_ENDLOC_UP, &endloc_up) ||
	    !EEP_READ(AR5211_EEP_ENDLOC_LO, &endloc_lo)) {
		fprintf(stderr, "Unable to read EEPROM size\n");
		return false;
	}

	if (endloc_up) {
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

	return true;
}

static bool eep_5211_check(struct atheepmgr *aem)
{
	return true;
}

const struct eepmap eepmap_5211 = {
	.name = "5211",
	.desc = "Legacy .11abg chips EEPROM map (AR5211/AR5212/AR5414/etc.)",
	.eep_buf_sz = AR5211_SIZE_MAX,
	.fill_eeprom = eep_5211_fill,
	.check_eeprom = eep_5211_check,
	.dump = {
	},
};
