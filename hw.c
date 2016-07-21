/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
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
#include "hw.h"

static struct {
	uint32_t version;
	uint32_t revision;
	const char * name;
} mac_bb_names[] = {
	/* .11abg legacy family */
	{ AR_SREV_VERSION_5211,	AR_SREV_REVISION_5211,	"5211" },
	{ AR_SREV_VERSION_5212,	AR_SREV_REVISION_5212,	"5212" },
	{ AR_SREV_VERSION_5213,	AR_SREV_REVISION_5213,	"5213" },
	{ AR_SREV_VERSION_5213A,AR_SREV_REVISION_5213A,	"5213A" },
	{ AR_SREV_VERSION_5413,	AR_SREV_REVISION_5413,	"5413" },
	{ AR_SREV_VERSION_5414,	AR_SREV_REVISION_5414,	"5414" },
	/* .11abgn family */
	{ AR_SREV_VERSION_5416,	AR_SREV_REVISION_5416,	"5416" },
	{ AR_SREV_VERSION_5418,	AR_SREV_REVISION_5418,	"5418" },
};

static struct {
	uint32_t version;
	const char * name;
} mac_bb_names2[] = {
	/* Devices with external radios */
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

static const char *mac_bb_name(uint32_t ver, uint32_t rev)
{
	const char *name = "????";
	int i;

	for (i = 0; i < ARRAY_SIZE(mac_bb_names); i++) {
		if (mac_bb_names[i].version == ver)
			name = mac_bb_names[i].name;
		else
			continue;

		if (mac_bb_names[i].revision == rev)
			break;	/* NB: name already assigned above */
	}

	return name;
}

static const char *mac_bb_name2(uint32_t mac_bb_version)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mac_bb_names2); i++) {
		if (mac_bb_names2[i].version == mac_bb_version) {
			return mac_bb_names2[i].name;
		}
	}

	return "????";
}

static void hw_read_revisions(struct atheepmgr *aem)
{
	uint32_t val = REG_READ(AR_SREV);

	if ((val & AR_SREV_ID) == 0xFF) {
		aem->macVersion = (val & AR_SREV_VERSION2) >> AR_SREV_TYPE2_S;
		aem->macRev = MS(val, AR_SREV_REVISION2);

		printf("Atheros AR%s MAC/BB Rev:%x (SREV: 0x%08x)\n",
		       mac_bb_name2(aem->macVersion), aem->macRev, val);
	} else {
		aem->macVersion = MS(val, AR_SREV_VERSION);
		aem->macRev = val & AR_SREV_REVISION;

		printf("Atheros AR%s MAC/BB (SREV: 0x%08x)\n",
		       mac_bb_name(aem->macVersion, aem->macRev), val);
	}
}

bool hw_wait(struct atheepmgr *aem, uint32_t reg, uint32_t mask,
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

static int hw_gpio_input_get_ar9xxx(struct atheepmgr *aem, unsigned gpio)
{
	uint32_t regval = REG_READ(AR9XXX_GPIO_IN_OUT);

	if (gpio >= aem->gpio_num)
		return 0;

	if (AR_SREV_9300_20_OR_LATER(aem))
		regval = MS(regval, AR9300_GPIO_IN_VAL);
	else if (AR_SREV_9287_11_OR_LATER(aem))
		regval = MS(regval, AR9287_GPIO_IN_VAL);
	else if (AR_SREV_9285_12_OR_LATER(aem))
		regval = MS(regval, AR9285_GPIO_IN_VAL);
	else if (AR_SREV_9280_20_OR_LATER(aem))
		regval = MS(regval, AR9280_GPIO_IN_VAL);
	else
		regval = MS(regval, AR5416_GPIO_IN_VAL);

	return !!(regval & BIT(gpio));
}

static int hw_gpio_output_get_ar9xxx(struct atheepmgr *aem, unsigned gpio)
{
	if (gpio >= aem->gpio_num)
		return 0;

	return !!(REG_READ(AR9XXX_GPIO_IN_OUT) & BIT(gpio));
}

static void hw_gpio_output_set_ar9xxx(struct atheepmgr *aem, unsigned gpio,
				      int val)
{
	REG_RMW(AR9XXX_GPIO_IN_OUT, !!val << gpio, 1 << gpio);
}

static int hw_gpio_out_mux_get_ar9xxx(struct atheepmgr *aem, unsigned gpio)
{
	uint32_t reg;
	unsigned sh = (gpio % 6) * 5;

	if (gpio >= aem->gpio_num)
		return 0;

	if (gpio > 11)
		reg = AR9XXX_GPIO_OUTPUT_MUX3;
	else if (gpio > 5)
		reg = AR9XXX_GPIO_OUTPUT_MUX2;
	else
		reg = AR9XXX_GPIO_OUTPUT_MUX1;

	return (REG_READ(reg) >> sh) & AR9XXX_GPIO_OUTPUT_MUX_MASK;
}

static void hw_gpio_out_mux_set_ar9xxx(struct atheepmgr *aem, unsigned gpio,
				       int type)
{
	uint32_t reg, tmp;
	unsigned sh = (gpio % 6) * 5;

	if (gpio >= aem->gpio_num)
		return;

	if (gpio > 11)
		reg = AR9XXX_GPIO_OUTPUT_MUX3;
	else if (gpio > 5)
		reg = AR9XXX_GPIO_OUTPUT_MUX2;
	else
		reg = AR9XXX_GPIO_OUTPUT_MUX1;

	if (AR_SREV_9280_20_OR_LATER(aem) ||
	    reg != AR9XXX_GPIO_OUTPUT_MUX1) {
		REG_RMW(reg, type << sh, AR9XXX_GPIO_OUTPUT_MUX_MASK << sh);
	} else {
		tmp = REG_READ(reg);
		tmp = ((tmp & 0x1f0) << 1) | (tmp & ~0x1f0);
		tmp &= ~(AR9XXX_GPIO_OUTPUT_MUX_MASK << sh);
		tmp |= type << sh;
		REG_WRITE(reg, tmp);
	}
}

static const char *hw_gpio_out_mux_get_str_ar9xxx(struct atheepmgr *aem,
						  unsigned gpio)
{
	int type = hw_gpio_out_mux_get_ar9xxx(aem, gpio);

	switch (type) {
	case AR9XXX_GPIO_OUTPUT_MUX_OUTPUT:
		return "Out";
	case AR9XXX_GPIO_OUTPUT_MUX_TX_FRAME:
		return "TxF";
	case AR9XXX_GPIO_OUTPUT_MUX_RX_CLEAR:
		return "RxC";
	case AR9XXX_GPIO_OUTPUT_MUX_MAC_NETWORK:
		return "Net";
	case AR9XXX_GPIO_OUTPUT_MUX_MAC_POWER:
		return "Pwr";
	}

	return "Unk";
}

static int hw_gpio_dir_get_ar9xxx(struct atheepmgr *aem, unsigned gpio)
{
	unsigned sh = gpio * 2;

	if (gpio >= aem->gpio_num)
		return -1;

	return (REG_READ(AR9XXX_GPIO_OE_OUT) >> sh) & AR9XXX_GPIO_OE_OUT_DRV;
}

static void hw_gpio_dir_set_out_ar9xxx(struct atheepmgr *aem, unsigned gpio)
{
	unsigned sh = gpio * 2;

	if (gpio >= aem->gpio_num)
		return;

	hw_gpio_out_mux_set_ar9xxx(aem, gpio, AR9XXX_GPIO_OUTPUT_MUX_OUTPUT);

	REG_RMW(AR9XXX_GPIO_OE_OUT,
		AR9XXX_GPIO_OE_OUT_DRV_ALL << sh,
		AR9XXX_GPIO_OE_OUT_DRV << sh);
}

static const char *hw_gpio_dir_get_str_ar9xxx(struct atheepmgr *aem,
					      unsigned gpio)
{
	int dir = hw_gpio_dir_get_ar9xxx(aem, gpio);

	switch (dir) {
	case AR9XXX_GPIO_OE_OUT_DRV_NO:
		return "In";
	case AR9XXX_GPIO_OE_OUT_DRV_LOW:
		return "Low";
	case AR9XXX_GPIO_OE_OUT_DRV_HI:
		return "Hi";
	case AR9XXX_GPIO_OE_OUT_DRV_ALL:
		return "Out";
	}

	return "Unk";
}

static const struct gpio_ops gpio_ops_ar9xxx = {
	.input_get = hw_gpio_input_get_ar9xxx,
	.output_get = hw_gpio_output_get_ar9xxx,
	.output_set = hw_gpio_output_set_ar9xxx,
	.dir_set_out = hw_gpio_dir_set_out_ar9xxx,
	.dir_get_str = hw_gpio_dir_get_str_ar9xxx,
	.out_mux_get_str = hw_gpio_out_mux_get_str_ar9xxx,
};

static bool hw_eeprom_read_9xxx(struct atheepmgr *aem, uint32_t off,
				uint16_t *data)
{
#define WAIT_MASK	AR_EEPROM_STATUS_DATA_BUSY | \
			AR_EEPROM_STATUS_DATA_PROT_ACCESS
#define WAIT_TIME	AH_WAIT_TIMEOUT

	(void)REG_READ(AR5416_EEPROM_OFFSET + (off << AR5416_EEPROM_S));

	if (!hw_wait(aem, AR_EEPROM_STATUS_DATA, WAIT_MASK, 0, WAIT_TIME))
		return false;

	*data = MS(REG_READ(AR_EEPROM_STATUS_DATA),
		   AR_EEPROM_STATUS_DATA_VAL);

	return true;

#undef WAIT_TIME
#undef WAIT_MASK
}

static bool hw_eeprom_write_9xxx(struct atheepmgr *aem, uint32_t off,
				 uint16_t data)
{
#define WAIT_MASK	AR_EEPROM_STATUS_DATA_BUSY | \
			AR_EEPROM_STATUS_DATA_BUSY_ACCESS | \
			AR_EEPROM_STATUS_DATA_PROT_ACCESS | \
			AR_EEPROM_STATUS_DATA_ABSENT_ACCESS
#define WAIT_TIME	AH_WAIT_TIMEOUT

	REG_WRITE(AR5416_EEPROM_OFFSET + (off << AR5416_EEPROM_S), data);
	if (!hw_wait(aem, AR_EEPROM_STATUS_DATA, WAIT_MASK, 0, WAIT_TIME))
		return false;

	return true;

#undef WAIT_TIME
#undef WAIT_MASK
}

static int hw_gpio_input_get_ar5xxx(struct atheepmgr *aem, unsigned gpio)
{
	if (gpio >= aem->gpio_num)
		return 0;

	return !!(REG_READ(AR5XXX_GPIO_IN) & BIT(gpio));
}

static int hw_gpio_output_get_ar5xxx(struct atheepmgr *aem, unsigned gpio)
{
	if (gpio >= aem->gpio_num)
		return 0;

	return !!(REG_READ(AR5XXX_GPIO_OUT) & BIT(gpio));
}

static void hw_gpio_output_set_ar5xxx(struct atheepmgr *aem, unsigned gpio,
				      int val)
{
	REG_RMW(AR5XXX_GPIO_OUT, !!val << gpio, 1 << gpio);
}

static int hw_gpio_dir_get_ar5xxx(struct atheepmgr *aem, unsigned gpio)
{
	unsigned sh = gpio * 2;

	if (gpio >= aem->gpio_num)
		return 0;

	return (REG_READ(AR5XXX_GPIO_CTRL) >> sh) & AR5XXX_GPIO_CTRL_DRV;
}

static void hw_gpio_dir_set_out_ar5xxx(struct atheepmgr *aem, unsigned gpio)
{
	unsigned sh = gpio * 2;

	if (gpio >= aem->gpio_num)
		return;

	REG_RMW(AR5XXX_GPIO_CTRL,
		AR5XXX_GPIO_CTRL_DRV_ALL << sh,
		AR5XXX_GPIO_CTRL_DRV << sh);
}

static const char *hw_gpio_dir_get_str_ar5xxx(struct atheepmgr *aem,
					      unsigned gpio)
{
	int dir = hw_gpio_dir_get_ar5xxx(aem, gpio);

	switch (dir) {
	case AR5XXX_GPIO_CTRL_DRV_NO:
		return "In";
	case AR5XXX_GPIO_CTRL_DRV_LOW:
		return "Low";
	case AR5XXX_GPIO_CTRL_DRV_HI:
		return "Hi";
	case AR5XXX_GPIO_CTRL_DRV_ALL:
		return "Out";
	}

	return "Unk";
}

static const struct gpio_ops gpio_ops_ar5xxx = {
	.input_get = hw_gpio_input_get_ar5xxx,
	.output_get = hw_gpio_output_get_ar5xxx,
	.output_set = hw_gpio_output_set_ar5xxx,
	.dir_set_out = hw_gpio_dir_set_out_ar5xxx,
	.dir_get_str = hw_gpio_dir_get_str_ar5xxx,
};

static bool hw_eeprom_read_5211(struct atheepmgr *aem, uint32_t off, uint16_t *data)
{
	static const uint32_t wait_to = AH_WAIT_TIMEOUT;
	uint32_t to, st;

	REG_WRITE(AR5211_EEPROM_ADDR, off);
	REG_WRITE(AR5211_EEPROM_CMD, AR5211_EEPROM_CMD_READ);

	for (to = 0; to < wait_to; ++to) {
		st = REG_READ(AR5211_EEPROM_STATUS);
		if (st & AR5211_EEPROM_STATUS_READ_COMPLETE) {
			if (st & AR5211_EEPROM_STATUS_READ_ERROR)
				return false;
			break;
		}
		usleep(AH_TIME_QUANTUM);
	}
	if (wait_to == to)
		return false;

	*data = REG_READ(AR5211_EEPROM_DATA) & 0xffff;

	return true;
}

static bool hw_eeprom_write_5211(struct atheepmgr *aem, uint32_t off, uint16_t data)
{
	return false;
}

static void hw_eeprom_lock_gpio(struct atheepmgr *aem, int lock)
{
	int val = !!aem->eep_wp_gpio_pol ^ !!lock;

	if (aem->eep_wp_gpio_num >= aem->gpio_num ||
	    aem->eep_wp_gpio_num < 0)
		return;

	if (!aem->gpio) {
		fprintf(stderr, "GPIO management is not available, EEPROM %s is impossible\n",
			lock ? "locking" : "unlocking");
		return;
	}

	aem->gpio->dir_set_out(aem, aem->eep_wp_gpio_num);
	usleep(1);
	aem->gpio->output_set(aem, aem->eep_wp_gpio_num, val);
	usleep(1);
}

static const struct eep_ops hw_eep_9xxx = {
	.read = hw_eeprom_read_9xxx,
	.write = hw_eeprom_write_9xxx,
	.lock = hw_eeprom_lock_gpio,
};

static const struct eep_ops hw_eep_5211 = {
	.read = hw_eeprom_read_5211,
	.write = hw_eeprom_write_5211,
	.lock = hw_eeprom_lock_gpio,
};

void hw_eeprom_set_ops(struct atheepmgr *aem)
{
	if (aem->con->eep) {
		printf("EEPROM access ops: use connector's ops\n");
		aem->eep = aem->con->eep;
	} else if (AR_SREV_5416_OR_LATER(aem)) {
		printf("EEPROM access ops: use AR9xxx ops\n");
		aem->eep = &hw_eep_9xxx;
	} else if (AR_SREV_5211_OR_LATER(aem)) {
		printf("EEPROM access ops: use AR5211 ops\n");
		aem->eep = &hw_eep_5211;
	} else {
		printf("Unable to select EEPROM access ops due to unknown chip\n");
	}
}

bool hw_eeprom_read(struct atheepmgr *aem, uint32_t off, uint16_t *data)
{
	if (!aem->eep || !aem->eep->read(aem, off, data))
		return false;

	if (aem->eep_io_swap)
		*data = bswap_16(*data);

	return true;
}

bool hw_eeprom_write(struct atheepmgr *aem, uint32_t off, uint16_t data)
{
	if (aem->eep_io_swap)
		data = bswap_16(data);

	if (!aem->eep || !aem->eep->write(aem, off, data))
		return false;

	return true;
}

void hw_eeprom_lock(struct atheepmgr *aem, int lock)
{
	if (aem->eep && aem->eep->lock)
		aem->eep->lock(aem, lock);
}

int hw_init(struct atheepmgr *aem)
{
	hw_read_revisions(aem);

	if (AR_SREV_5416_OR_LATER(aem)) {
		aem->gpio = &gpio_ops_ar9xxx;

		if (AR_SREV_9300_20_OR_LATER(aem))
			aem->gpio_num = 17;
		else if (AR_SREV_9287_11_OR_LATER(aem))
			aem->gpio_num = 11;
		else if (AR_SREV_9285_12_OR_LATER(aem))
			aem->gpio_num = 12;
		else if (AR_SREV_9280_20_OR_LATER(aem))
			aem->gpio_num = 10;
		else
			aem->gpio_num = 14;
	} else if (AR_SREV_5211_OR_LATER(aem)) {
		aem->gpio = &gpio_ops_ar5xxx;
		aem->gpio_num = 6;
	} else {
		fprintf(stderr, "Unable to configure chip GPIO support\n");
	}

	if (aem->eep_wp_gpio_num == EEP_WP_GPIO_AUTO) {
		if (AR_SREV_5416_OR_LATER(aem)) {
			aem->eep_wp_gpio_num = 3;
			aem->eep_wp_gpio_pol = 0;
		} else if (AR_SREV_5211_OR_LATER(aem)) {
			aem->eep_wp_gpio_num = 4;
			aem->eep_wp_gpio_pol = 0;
		} else {
			fprintf(stderr, "Unable to determine EEPROM unlocking GPIO, the feature will be disabled\n");
			aem->eep_wp_gpio_num = EEP_WP_GPIO_NONE;
		}
	}

	return 0;
}
