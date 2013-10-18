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

static struct {
	uint32_t version;
	const char * name;
} mac_bb_names[] = {
	/* Devices with external radios */
	{ AR_SREV_VERSION_5416_PCI,	"5416" },
	{ AR_SREV_VERSION_5416_PCIE,	"5418" },
	{ AR_SREV_VERSION_9160,		"9160" },
	/* Single-chip solutions */
	{ AR_SREV_VERSION_9280,		"9280" },
	{ AR_SREV_VERSION_9285,		"9285" },
	{ AR_SREV_VERSION_9287,         "9287" },
	{ AR_SREV_VERSION_9300,         "9300" },
	{ AR_SREV_VERSION_9330,         "9330" },
	{ AR_SREV_VERSION_9485,         "9485" },
	{ AR_SREV_VERSION_9462,         "9462" },
	{ AR_SREV_VERSION_9565,         "9565" },
	{ AR_SREV_VERSION_9340,         "9340" },
	{ AR_SREV_VERSION_9550,         "9550" },
};

static const char *mac_bb_name(uint32_t mac_bb_version)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mac_bb_names); i++) {
		if (mac_bb_names[i].version == mac_bb_version) {
			return mac_bb_names[i].name;
		}
	}

	return "????";
}

void hw_read_revisions(struct edump *edump)
{
	uint32_t val;

	val = REG_READ(AR_SREV) & AR_SREV_ID;

	if (val == 0xFF) {
		val = REG_READ(AR_SREV);
		edump->macVersion = (val & AR_SREV_VERSION2) >> AR_SREV_TYPE2_S;
		edump->macRev = MS(val, AR_SREV_REVISION2);
	} else {
		edump->macVersion = MS(val, AR_SREV_VERSION);
		edump->macRev = val & AR_SREV_REVISION;
	}

	printf("Atheros AR%s MAC/BB Rev:%x\n",
	       mac_bb_name(edump->macVersion), edump->macRev);
}

bool hw_wait(struct edump *edump, uint32_t reg, uint32_t mask,
	     uint32_t val, uint32_t timeout)
{
	int i;

	for (i = 0; i < (timeout / AH_TIME_QUANTUM); i++) {
		if ((REG_READ(reg) & mask) == val)
			return true;

		usleep(AH_TIME_QUANTUM);
	}

	return false;
}

bool hw_eeprom_read_9xxx(struct edump *edump, uint32_t off, uint16_t *data)
{
#define WAIT_MASK	AR_EEPROM_STATUS_DATA_BUSY | \
			AR_EEPROM_STATUS_DATA_PROT_ACCESS
#define WAIT_TIME	AH_WAIT_TIMEOUT

	(void)REG_READ(AR5416_EEPROM_OFFSET + (off << AR5416_EEPROM_S));

	if (!hw_wait(edump, AR_EEPROM_STATUS_DATA, WAIT_MASK, 0, WAIT_TIME))
		return false;

	*data = MS(REG_READ(AR_EEPROM_STATUS_DATA),
		   AR_EEPROM_STATUS_DATA_VAL);

	return true;

#undef WAIT_TIME
#undef WAIT_MASK
}

bool hw_eeprom_write_9xxx(struct edump *edump, uint32_t off, uint16_t data)
{
#define WAIT_MASK	AR_EEPROM_STATUS_DATA_BUSY | \
			AR_EEPROM_STATUS_DATA_BUSY_ACCESS | \
			AR_EEPROM_STATUS_DATA_PROT_ACCESS | \
			AR_EEPROM_STATUS_DATA_ABSENT_ACCESS
#define WAIT_TIME	AH_WAIT_TIMEOUT

	REG_WRITE(AR5416_EEPROM_OFFSET + (off << AR5416_EEPROM_S), data);
	if (!hw_wait(edump, AR_EEPROM_STATUS_DATA, WAIT_MASK, 0, WAIT_TIME))
		return false;

	return true;

#undef WAIT_TIME
#undef WAIT_MASK
}

bool hw_eeprom_read(struct edump *edump, uint32_t off, uint16_t *data)
{
	if (!edump->con->eep_read(edump, off, data))
		return false;

	if (edump->eep_io_swap)
		*data = bswap_16(*data);

	return true;
}

bool hw_eeprom_write(struct edump *edump, uint32_t off, uint16_t data)
{
	if (edump->eep_io_swap)
		data = bswap_16(data);

	if (!edump->con->eep_write(edump, off, data))
		return false;

	return true;
}
