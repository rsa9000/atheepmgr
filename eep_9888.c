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

#include "atheepmgr.h"
#include "utils.h"
#include "eep_common.h"
#include "eep_9888.h"

struct eep_9888_priv {
	int curr_ref_tpl;		/* Current reference EEPROM template */
	struct qca9888_eeprom eep;
};

static bool eep_9888_load_blob(struct atheepmgr *aem)
{
	const int data_size = sizeof(struct qca9888_eeprom);
	struct eep_9888_priv *emp = aem->eepmap_priv;
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

static int eep_9888_check(struct atheepmgr *aem)
{
	struct eep_9888_priv *emp = aem->eepmap_priv;
	struct qca9888_eeprom *eep = &emp->eep;
	struct qca9888_base_eep_hdr *pBase = &eep->baseEepHeader;
	uint16_t sum;

	if (pBase->length != sizeof(*eep) &&
	    bswap_16(pBase->length) != sizeof(*eep)) {
		fprintf(stderr, "Bad EEPROM length 0x%04x/0x%04x (expect 0x%04x)\n",
			pBase->length, bswap_16(pBase->length),
			(unsigned int)sizeof(*eep));
		return false;
	}

	/**
	 * NB: take pointer another one time from container to avoid warning
	 * about a *possible* unaligned access
	 */
	sum = eep_calc_csum((uint16_t *)&emp->eep,
			    sizeof(emp->eep) / sizeof(uint16_t));
	if (sum != 0xffff) {
		fprintf(stderr, "Bad EEPROM checksum 0x%04x\n", sum);
		return false;
	}

	return true;
}

static void eep_9888_dump_base_header(struct atheepmgr *aem)
{
	const struct eep_9888_priv *emp = aem->eepmap_priv;
	const struct qca9888_eeprom *eep = &emp->eep;
	const struct qca9888_base_eep_hdr *pBase = &eep->baseEepHeader;

	EEP_PRINT_SECT_NAME("EEPROM Base Header");

	printf("%-30s : 0x%04X\n", "Length", pBase->length);
	printf("%-30s : 0x%04X\n", "Checksum", pBase->checksum);
	printf("%-30s : %d\n", "EEP Version", pBase->eepromVersion);
	printf("%-30s : %d\n", "Template Version", pBase->templateVersion);
	printf("%-30s : %02X:%02X:%02X:%02X:%02X:%02X\n",
	       "MacAddress",
	       pBase->macAddr[0], pBase->macAddr[1], pBase->macAddr[2],
	       pBase->macAddr[3], pBase->macAddr[4], pBase->macAddr[5]);

	printf("\nCustomer Data in hex:\n");
	hexdump_print(pBase->custData, sizeof(pBase->custData));

	printf("\n");
}

const struct eepmap eepmap_9888 = {
	.name = "9888",
	.desc = "EEPROM map for .11ac chips (QCA9884/QCA9886/QCA9888/IPQ4018/IPQ4019/etc.)",
	.chip_regs = {
		.srev = 0x800ec,
	},
	.priv_data_sz = sizeof(struct eep_9888_priv),
	.eep_buf_sz = sizeof(struct qca9888_eeprom) / sizeof(uint16_t),
	.load_blob = eep_9888_load_blob,
	.check_eeprom = eep_9888_check,
	.dump = {
		[EEP_SECT_BASE] = eep_9888_dump_base_header,
	},
};
