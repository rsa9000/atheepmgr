/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * Copyright (c) 2013,2016-2021 Sergey Ryazanov <ryazanov.s.a@gmail.com>
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

#include <limits.h>

#include "atheepmgr.h"
#include "utils.h"

static struct atheepmgr __aem;

static const struct eepmap * const eepmaps[] = {
	&eepmap_5211,
	&eepmap_5416,
	&eepmap_6174,
	&eepmap_9285,
	&eepmap_9287,
	&eepmap_9300,
	&eepmap_9880,
	&eepmap_9888,
};

#define AEM_CHIP(__name, __eepmap)		\
		.name = __name, .eepmap = __eepmap
#define AEM_CHIP_EEP5211(__name)		\
		AEM_CHIP(__name, &eepmap_5211)
#define AEM_CHIP_EEP5416(__name)		\
		AEM_CHIP(__name, &eepmap_5416)
#define AEM_CHIP_EEP6174(__name)		\
		AEM_CHIP(__name, &eepmap_6174)
#define AEM_CHIP_EEP9285(__name)		\
		AEM_CHIP(__name, &eepmap_9285)
#define AEM_CHIP_EEP9287(__name)		\
		AEM_CHIP(__name, &eepmap_9287)
#define AEM_CHIP_EEP9300(__name)		\
		AEM_CHIP(__name, &eepmap_9300)
#define AEM_CHIP_EEP9880(__name)		\
		AEM_CHIP(__name, &eepmap_9880)
#define AEM_CHIP_EEP9888(__name)		\
		AEM_CHIP(__name, &eepmap_9888)

static const struct chip chips[] = {
	/* AR5211 EEPROM map PCI/PCIe chip(s) */
	{ AEM_CHIP_EEP5211("AR5211"), .pciids = {{ .dev_id = 0x0012 }} },
	{ AEM_CHIP_EEP5211("AR5212"), .pciids = {{ .dev_id = 0x0013 }} },
	{ AEM_CHIP_EEP5211("AR5213"), .pciids = {{ .dev_id = 0x0013 }} },
	{ AEM_CHIP_EEP5211("AR2413"), .pciids = {{ .dev_id = 0x001a }} },
	{ AEM_CHIP_EEP5211("AR2414"), .pciids = {{ .dev_id = 0x0013 }} },
	{ AEM_CHIP_EEP5211("AR2415"), .pciids = {{ .dev_id = 0x001b }} },
	{ AEM_CHIP_EEP5211("AR5413"), .pciids = {{ .dev_id = 0x001b }} },
	{ AEM_CHIP_EEP5211("AR5414"), .pciids = {{ .dev_id = 0x001b }} },
	{ AEM_CHIP_EEP5211("AR2417"), .pciids = {{ .dev_id = 0x001d }} },
	{ AEM_CHIP_EEP5211("AR2423"), .pciids = {{ .dev_id = 0x001c }} },
	{ AEM_CHIP_EEP5211("AR2424"), .pciids = {{ .dev_id = 0x001c }} },
	{ AEM_CHIP_EEP5211("AR2425"), .pciids = {{ .dev_id = 0x001c }} },
	{ AEM_CHIP_EEP5211("AR5423"), .pciids = {{ .dev_id = 0x001c }} },
	{ AEM_CHIP_EEP5211("AR5424"), .pciids = {{ .dev_id = 0x001c }} },
	/* AR5211 EEPROM map WiSoC(s) */
	{ AEM_CHIP_EEP5211("AR5311") },
	{ AEM_CHIP_EEP5211("AR5312") },
	{ AEM_CHIP_EEP5211("AR2312") },
	{ AEM_CHIP_EEP5211("AR2313") },
	{ AEM_CHIP_EEP5211("AR2315") },
	{ AEM_CHIP_EEP5211("AR2316") },
	{ AEM_CHIP_EEP5211("AR2317") },
	{ AEM_CHIP_EEP5211("AR2318") },

	/* AR5416 EEPROM map PCI/PCIe chip(s) */
	{ AEM_CHIP_EEP5416("AR5416"), .pciids = {{ .dev_id = 0x0023 }} },
	{ AEM_CHIP_EEP5416("AR5418"), .pciids = {{ .dev_id = 0x0024 }} },
	{ AEM_CHIP_EEP5416("AR9160"), .pciids = {{ .dev_id = 0x0027 }} },
	{ AEM_CHIP_EEP5416("AR9220"), .pciids = {{ .dev_id = 0x0029 }} },
	{ AEM_CHIP_EEP5416("AR9223"), .pciids = {{ .dev_id = 0x0029 }} },
	{ AEM_CHIP_EEP5416("AR9280"), .pciids = {{ .dev_id = 0x002a }} },
	{ AEM_CHIP_EEP5416("AR9281"), .pciids = {{ .dev_id = 0x002a }} },
	{ AEM_CHIP_EEP5416("AR9283"), .pciids = {{ .dev_id = 0x002a }} },
	/* AR5416 EEPROM map WiSoC (AHB interface) chip(s) */
	{ AEM_CHIP_EEP5416("AR9130") },
	{ AEM_CHIP_EEP5416("AR9132") },

	/* AR9285 EEPROM map PCIe chip(s) */
	{ AEM_CHIP_EEP9285("AR2427"), .pciids = {{ .dev_id = 0x002c }} },	/* Check PCI Id */
	{ AEM_CHIP_EEP9285("AR9285"), .pciids = {{ .dev_id = 0x002b }} },

	/* AR9287 EEPROM map PCI/PCIe chip(s) */
	{ AEM_CHIP_EEP9287("AR9227"), .pciids = {{ .dev_id = 0x002d }} },
	{ AEM_CHIP_EEP9287("AR9287"), .pciids = {{ .dev_id = 0x002e }} },

	/* AR93xx EEPROM map PCIe chip(s) */
	{ AEM_CHIP_EEP9300("AR9380"), .pciids = {{ .dev_id = 0x0030 }} },
	{ AEM_CHIP_EEP9300("AR9381"), .pciids = {{ .dev_id = 0x0030 }} },
	{ AEM_CHIP_EEP9300("QCA9381"), .pciids = {{ .dev_id = 0x0030 }} },
	{ AEM_CHIP_EEP9300("AR9382"), .pciids = {{ .dev_id = 0x0030 }} },
	{ AEM_CHIP_EEP9300("AR9388"), .pciids = {{ .dev_id = 0x0030 }} },	/* Check PCI Id */
	{ AEM_CHIP_EEP9300("AR9390"), .pciids = {{ .dev_id = 0x0030 }} },	/* Check PCI Id */
	{ AEM_CHIP_EEP9300("AR9392"), .pciids = {{ .dev_id = 0x0030 }} },	/* Check PCI Id */
	{ AEM_CHIP_EEP9300("AR9462"), .pciids = {{ .dev_id = 0x0034 }} },
	{ AEM_CHIP_EEP9300("AR9463"), .pciids = {{ .dev_id = 0x0034 }} },	/* Check PCI Id */
	{ AEM_CHIP_EEP9300("AR9485"), .pciids = {{ .dev_id = 0x0032 }} },
	{ AEM_CHIP_EEP9300("AR9580"), .pciids = {{ .dev_id = 0x0033 }} },
	{ AEM_CHIP_EEP9300("QCA9580"), .pciids = {{ .dev_id = 0x0033 }} },
	{ AEM_CHIP_EEP9300("AR9582"), .pciids = {{ .dev_id = 0x0033 }} },
	{ AEM_CHIP_EEP9300("QCA9582"), .pciids = {{ .dev_id = 0x0033 }} },
	{ AEM_CHIP_EEP9300("AR9590"), .pciids = {{ .dev_id = 0x0033 }} },
	{ AEM_CHIP_EEP9300("QCA9590"), .pciids = {{ .dev_id = 0x0033 }} },
	{ AEM_CHIP_EEP9300("AR9592"), .pciids = {{ .dev_id = 0x0033 }} },	/* Check PCI Id */
	{ AEM_CHIP_EEP9300("QCA9592"), .pciids = {{ .dev_id = 0x0033 }} },	/* Check PCI Id */
	{ AEM_CHIP_EEP9300("QCA9565"), .pciids = {{ .dev_id = 0x0036 }} },
	{ AEM_CHIP_EEP9300("AR1111"), .pciids = {{ .dev_id = 0x0037 }} },
	/* AR93xx EEPROM map WiSoC (AHB interface) chip(s) */
	{ AEM_CHIP_EEP9300("AR9331") },
	{ AEM_CHIP_EEP9300("AR9341") },
	{ AEM_CHIP_EEP9300("AR9342") },
	{ AEM_CHIP_EEP9300("AR9344") },
	{ AEM_CHIP_EEP9300("AR9350") },

	/* QCA988x EEPROM map PCIe chip(s) */
	{ AEM_CHIP_EEP9880("QCA9860"), .pciids = {{ .dev_id = 0x003c }} },
	{ AEM_CHIP_EEP9880("QCA9862"), .pciids = {{ .dev_id = 0x003c }} },
	{ AEM_CHIP_EEP9880("QCA9880"), .pciids = {{ .dev_id = 0x003c }} },
	{ AEM_CHIP_EEP9880("QCA9882"), .pciids = {{ .dev_id = 0x003c }} },
	{ AEM_CHIP_EEP9880("QCA9890"), .pciids = {{ .dev_id = 0x003c }} },
	{ AEM_CHIP_EEP9880("QCA9892"), .pciids = {{ .dev_id = 0x003c }} },

	/* QCA6174 EEPROM map PCIe chip(s) */
	{ AEM_CHIP_EEP6174("QCA6164"), .pciids = {{ .dev_id = 0x0041 }} },	/* Check PCI Id */
	{ AEM_CHIP_EEP6174("QCA6174"), .pciids = {{ .dev_id = 0x003e }} },

	/* QCA9888 EEPROM map PCIe chip(s) */
	{ AEM_CHIP_EEP9888("QCA9886"), .pciids = {{ .dev_id = 0x0056 }} },
	{ AEM_CHIP_EEP9888("QCA9888"), .pciids = {{ .dev_id = 0x0056 }} },
	{ AEM_CHIP_EEP9888("QCA9896"), .pciids = {{ .dev_id = 0x0056 }} },	/* Check PCI Id */
	{ AEM_CHIP_EEP9888("QCA9898"), .pciids = {{ .dev_id = 0x0056 }} },	/* Check PCI Id */
	/* QCA9888 EEPROM map WiSoC (AHB interface) chip(s) */
	{ AEM_CHIP_EEP9888("IPQ4018") },
	{ AEM_CHIP_EEP9888("IPQ4019") },
	{ AEM_CHIP_EEP9888("IPQ4028") },
	{ AEM_CHIP_EEP9888("IPQ4029") },
};

int chips_find_by_pci_id(uint16_t dev_id, const struct chip *res[], int nmemb)
{
	int i, j, n = 0;

	for (i = 0; i < ARRAY_SIZE(chips); ++i) {
		for (j = 0; j < ARRAY_SIZE(chips[0].pciids); ++j) {
			if (!chips[i].pciids[j].dev_id)
				break;
			if (chips[i].pciids[j].dev_id != dev_id)
				continue;
			if (n < nmemb)
				res[n++] = &chips[i];
			break;
		}
	}

	return n;
}

static const struct eepmap *eepmap_find_by_chip(const char *name)
{
	const struct chip *chip;
	int i, pci_dev_id;
	char *endp;

	if (strncasecmp("pci:", name, 4) == 0)
		goto search_by_pci_id;

	for (i = 0; i < ARRAY_SIZE(chips); ++i) {
		if (strcasecmp(chips[i].name, name) == 0)
			return chips[i].eepmap;
	}

	return NULL;

search_by_pci_id:
	name += 4;	/* Skip 'PCI:' prefix */
	errno = 0;
	pci_dev_id = strtoul(name, &endp, 16);
	if (pci_dev_id <= 0 || pci_dev_id > 0xffff || *endp != '\0' || errno) {
		fprintf(stderr, "Invalid PCI Device ID string format -- %s\n", name);
		return NULL;
	}

	if (chips_find_by_pci_id(pci_dev_id, &chip, 1) == 0)
		return NULL;

	return chip->eepmap;
}

static const struct eepmap *eepmap_find_by_name(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(eepmaps); ++i) {
		if (strcasecmp(eepmaps[i]->name, name) == 0)
			return eepmaps[i];
	}

	return NULL;
}

static const struct eepmap_section {
	const char *name;
	const char *desc;
} eepmap_sections_list[EEP_SECT_MAX] = {
	[EEP_SECT_INIT] = {
		.name = "init",
		.desc = "Device initialization information (e.g. PCI IDs)",
	},
	[EEP_SECT_BASE] = {
		.name = "base",
		.desc = "Main device configuration (common for all modes)",
	},
	[EEP_SECT_MODAL] = {
		.name = "modal",
		.desc = "Per-band (per-mode) device configuration",
	},
	[EEP_SECT_POWER] = {
		.name = "power",
		.desc = "Tx Power information (calibrations and limitations)",
	},
};

static int act_eep_dump(struct atheepmgr *aem, int argc, char *argv[])
{
	static char def[4] = {'a', 'l', 'l', '\0'};
	const struct eepmap *eepmap = aem->eepmap;
	char *list = argc > 0 ? argv[0] : def;
	int dump_mask = 0;
	char *tok, *p;
	int i;

	for (tok = strtok(list, ","); tok; tok = strtok(NULL, ",")) {
		for (; *tok == ' '; tok++);	/* Trim left */
		p = tok + strlen(tok) - 1;
		for (; *p == ' '; *(p--) = '\0');/* Trim right */

		if (tok[0] == '\0')
			continue;

		if (strcasecmp(tok, "all") == 0) {
			dump_mask = ~0;
			break;
		}
		if (strcasecmp(tok, "none") == 0)
			return 0;

		for (i = 0; i < EEP_SECT_MAX; ++i) {
			if (!eepmap_sections_list[i].name)
				continue;
			if (strcasecmp(tok, eepmap_sections_list[i].name) == 0)
				break;
		}
		if (i == EEP_SECT_MAX) {
			fprintf(stderr, "Unknown EEPROM section to dump -- %s\n",
				tok);
			return -EINVAL;
		} else if (!eepmap->dump[i]) {
			fprintf(stderr, "%s EEPROM map does not support %s section dumping\n",
				eepmap->name, eepmap_sections_list[i].name);
			continue;	/* Just ignore without interruption */
		}

		dump_mask |= 1 << i;
	}

	for (i = 0; i < EEP_SECT_MAX; ++i) {
		if (!(dump_mask & (1 << i)))
			continue;
		if (!eepmap->dump[i])
			continue;

		eepmap->dump[i](aem);
	}

	return 0;
}

static int act_eep_save(struct atheepmgr *aem, int argc, char *argv[])
{
	FILE *fp;
	const uint16_t *buf = aem->eep_buf;
	int eep_len = aem->eep_len;
	size_t res;

	if (!aem->eepmap->eep_buf_sz) {
		fprintf(stderr, "EEPROM map does not support buffered operation, so the content saving is not possible\n");
		return -EOPNOTSUPP;
	}

	if (argc < 1) {
		fprintf(stderr, "Output file for EEPROM saving is not specified, aborting\n");
		return -EINVAL;
	}

	fp = fopen(argv[0], "wb");
	if (!fp) {
		fprintf(stderr, "Unable to open output file for writing: %s\n",
			strerror(errno));
		return -errno;
	}

	res = fwrite(buf, sizeof(buf[0]), eep_len, fp);
	if (res != eep_len)
		fprintf(stderr, "Unable to save whole EEPROM contents: %s\n",
			strerror(errno));

	fclose(fp);

	return res == eep_len ? 0 : -EIO;
}

static int act_eep_unpack(struct atheepmgr *aem, int argc, char *argv[])
{
	const struct eepmap *eepmap = aem->eepmap;
	size_t res, data_len = aem->unpacked_len;
	const uint8_t *buf = aem->unpacked_buf;
	FILE *fp;

	if (!eepmap->unpacked_buf_sz) {
		fprintf(stderr, "EEPROM map does not support unpacked data saving\n");
		return -EOPNOTSUPP;
	}

	if (argc < 1) {
		fprintf(stderr, "Output file for unpacked data saving is not specified, aborting\n");
		return -EINVAL;
	}

	if (!data_len) {
		fprintf(stderr, "There are no unpacked data were produced, possibly data were not packed\n");
		return -ENOENT;
	}

	fp = fopen(argv[0], "wb");
	if (!fp) {
		fprintf(stderr, "Unable to open output file for writing: %s\n",
			strerror(errno));
		return -errno;
	}

	res = fwrite(buf, 1, data_len, fp);
	if (res != data_len)
		fprintf(stderr, "Unable to save unpacked data: %s\n",
			strerror(errno));

	fclose(fp);

	return res == data_len ? 0 : -EIO;
}

static const struct eepmap_param {
	int id;
	const char *name;
	const char *arg;
	const char *desc;
} eepmap_params_list[] = {
	{
		.id = EEP_UPDATE_MAC,
		.name = "mac",
		.arg = "<addr>",
		.desc = "Update device MAC address",
#ifdef CONFIG_I_KNOW_WHAT_I_AM_DOING
	}, {
		.id = EEP_ERASE_CTL,
		.name = "erasectl",
		.arg = NULL,
		.desc = "Erase CTL (Conformance Test Limit) data",
#endif
	}, {
		.name = NULL,
	}
};

static int act_eep_update(struct atheepmgr *aem, int argc, char *argv[])
{
	const struct eepmap *eepmap = aem->eepmap;
	const struct eepmap_param *param;
	char *val;
	int namelen;
	uint8_t macaddr[6];
	void *data;
	bool res;

	if (!eepmap->update_eeprom || !eepmap->params_mask) {
		fprintf(stderr, "EEPROM map does not support content updation, aborting\n");
		return -EOPNOTSUPP;
	}

	if (argc < 1) {
		fprintf(stderr, "Parameter for updation is not specified, aborting\n");
		return -EINVAL;
	}

	val = strchr(argv[0], '=');
	if (val) {
		namelen = val - argv[0];
		val = val[1] == '\0' ? NULL : val + 1;
	} else {
		namelen = strlen(argv[0]);
	}

	for (param = &eepmap_params_list[0]; param->name; ++param) {
		if (strncasecmp(param->name, argv[0], namelen) == 0)
			break;
	}
	if (!param->name) {
		fprintf(stderr, "Unknown parameter name -- %.*s\n", namelen,
			argv[0]);
		return -EINVAL;
	} else if (!(eepmap->params_mask & BIT(param->id))) {
		fprintf(stderr, "EEPROM map does not support parameter -- %.*s\n",
			namelen, argv[0]);
		return -EINVAL;
	}

	switch (param->id) {
	case EEP_UPDATE_MAC:
		if (!val) {
			fprintf(stderr, "MAC address updation requires an argument, aborting\n");
			return -EINVAL;
		} else if (macaddr_parse(val, macaddr) != 0) {
			fprintf(stderr, "Can not parse MAC address - %s\n",
				val);
			return -EINVAL;
		} else if(!macaddr_is_valid(macaddr)) {
			fprintf(stderr, "Invalid MAC address - %s\n", val);
			return -EINVAL;
		}
		data = macaddr;
		break;
	default:
		data = val;
	}

	EEP_UNLOCK();

	res = eepmap->update_eeprom(aem, param->id, data);

	EEP_LOCK();

	return res ? 0 : -EIO;
}

static int act_eep_tpl_export(struct atheepmgr *aem, int argc, char *argv[])
{
	const struct eepmap *eepmap = aem->eepmap;
	size_t res, data_len;
	const struct eeptemplate *tpl;
	unsigned long tplid;
	char *endp;
	FILE *fp;

	if (!eepmap) {
		fprintf(stderr, "EEPROM map is not specified, aborting\n");
		return -EINVAL;
	}

	data_len = eepmap->unpacked_buf_sz;
	if (!eepmap->templates || !data_len) {
		fprintf(stderr, "EEPROM map does not have any templates\n");
		return -EOPNOTSUPP;
	}

	if (argc < 1) {
		fprintf(stderr, "Template Name or Id is not specified, aborting\n");
		return -EINVAL;
	} else if (argc < 2) {
		fprintf(stderr, "Output file for template export is not specified, aborting\n");
		return -EINVAL;
	}

	errno = 0;
	tplid = strtoul(argv[0], &endp, 0);
	if (errno != 0 || *endp != '\0')
		tplid = ULONG_MAX;

	for (tpl = eepmap->templates; tpl->name; ++tpl) {
		if (tpl->id == tplid || strcasecmp(tpl->name, argv[0]) == 0)
			break;
	}
	if (!tpl->name) {
		fprintf(stderr, "Unknown template -- %s\n", argv[0]);
		return -EINVAL;
	}

	fp = fopen(argv[1], "wb");
	if (!fp) {
		fprintf(stderr, "Unable to open output file for template export: %s\n",
			strerror(errno));
		return -errno;
	}

	res = fwrite(tpl->data, 1, data_len, fp);
	if (res != data_len)
		fprintf(stderr, "Unable to save template data: %s\n",
			strerror(errno));

	fclose(fp);

	return res == data_len ? 0 : -EIO;
}

static int act_gpio_dump(struct atheepmgr *aem, int argc, char *argv[])
{
#define FOR_EACH_GPIO(_caption)				\
		printf("%20s:", _caption);		\
		for (i = 0; i < aem->gpio_num; ++i)
	int i;

	if (!aem->gpio) {
		fprintf(stderr, "GPIO control is not supported for this chip, aborting\n");
		return -EOPNOTSUPP;
	}

	FOR_EACH_GPIO("GPIO #")
		printf(" %-3u", i);
	printf("\n");
	FOR_EACH_GPIO("Direction")
		printf(" %-3s", aem->gpio->dir_get_str(aem, i));
	printf("\n");
	if (aem->gpio->out_mux_get_str) {
		FOR_EACH_GPIO("Output mux")
			printf(" %-3s", aem->gpio->out_mux_get_str(aem, i));
		printf("\n");
	}
	FOR_EACH_GPIO("Input value")
		printf(" %c  ", aem->gpio->input_get(aem, i) ? '1' : ' ');
	printf("\n");
	FOR_EACH_GPIO("Output value")
		printf(" %c  ", aem->gpio->output_get(aem, i) ? '1' : ' ');
	printf("\n");

	return 0;

#undef FOR_EACH_GPIO
}

static int act_reg_read(struct atheepmgr *aem, int argc, char *argv[])
{
	unsigned long addr;
	char *endp;
	uint32_t val;

	if (argc < 1) {
		fprintf(stderr, "Register address is not specified, aborting\n");
		return -EINVAL;
	}

	errno = 0;
	addr = strtoul(argv[0], &endp, 16);
	if (errno != 0 || *endp != '\0' || addr % 4 != 0) {
		fprintf(stderr, "Invalid register address -- %s\n", argv[0]);
		return -EINVAL;
	}

	val = REG_READ(addr);

	printf("0x%08lx: 0x%08lx\n", addr, (unsigned long)val);

	return 0;
}

static int act_reg_write(struct atheepmgr *aem, int argc, char *argv[])
{
	unsigned long addr, val;
	char *endp;

	if (argc < 2) {
		fprintf(stderr, "Register address and (or) value are not specified, aborting\n");
		return -EINVAL;
	}

	errno = 0;

	addr = strtoul(argv[0], &endp, 16);
	if (errno != 0 || *endp != '\0' || addr % 4 != 0) {
		fprintf(stderr, "Invalid register address -- %s\n", argv[0]);
		return -EINVAL;
	}

	val = strtoul(argv[1], &endp, 16);
	if (errno != 0 || *endp != '\0') {
		fprintf(stderr, "Invalid register value -- %s\n", argv[1]);
		return -EINVAL;
	}

	REG_WRITE(addr, val);

	return 0;
}

#define ACT_F_DATA	(1 << 0)	/* Action will interact with EEPROM/OTP data */
#define ACT_F_HW	(1 << 1)	/* Action require direct HW access */
#define ACT_F_AUTONOMOUS (1 << 2)	/* Action do not require input data or HW */
#define ACT_F_RAW_EEP	(1 << 3)	/* Action needs only raw EEPROM contents */
#define ACT_F_RAW_OTP	(1 << 4)	/* Action needs only raw OTP contents */
#define ACT_F_RAW_DATA	(ACT_F_RAW_EEP | ACT_F_RAW_OTP)

static const struct action {
	const char *name;
	int (*func)(struct atheepmgr *aem, int argc, char *argv[]);
	int flags;
} actions[] = {
	{
		.name = "dump",
		.func = act_eep_dump,
		.flags = ACT_F_DATA,
	}, {
		.name = "save",
		.func = act_eep_save,
		.flags = ACT_F_DATA,
	}, {
		.name = "saveraw",
		.func = act_eep_save,
		.flags = ACT_F_DATA | ACT_F_RAW_EEP | ACT_F_RAW_OTP,
	}, {
		.name = "saveraweep",
		.func = act_eep_save,
		.flags = ACT_F_DATA | ACT_F_RAW_EEP,
	}, {
		.name = "saverawotp",
		.func = act_eep_save,
		.flags = ACT_F_DATA | ACT_F_RAW_OTP,
	}, {
		.name = "unpack",
		.func = act_eep_unpack,
		.flags = ACT_F_DATA,
	}, {
		.name = "update",
		.func = act_eep_update,
		.flags = ACT_F_DATA,
	}, {
		.name = "templateexport",
		.func = act_eep_tpl_export,
		.flags = ACT_F_AUTONOMOUS,
	}, {
		.name = "gpiodump",
		.func = act_gpio_dump,
		.flags = ACT_F_HW,
	}, {
		.name = "regread",
		.func = act_reg_read,
		.flags = ACT_F_HW,
	}, {
		.name = "regwrite",
		.func = act_reg_write,
		.flags = ACT_F_HW,
	}
};

#define CON_USAGE_FILE		"-F <eepdump>"
#if defined(CONFIG_CON_MEM)
#define CON_USAGE_MEM		" | -M <ioaddr>"
#define CON_OPTSTR_MEM		"M:"
#else
#define CON_USAGE_MEM		""
#define CON_OPTSTR_MEM		""
#endif
#if defined(CONFIG_CON_PCI)
#define CON_USAGE_PCI		" | -P <slot>"
#define CON_OPTSTR_PCI		"P:"
#else
#define CON_USAGE_PCI		""
#define CON_OPTSTR_PCI		""
#endif
#if defined(CONFIG_CON_DRIVER)
#define CON_USAGE_DRIVER	" | -D <dev>"
#define CON_OPTSTR_DRIVER	"D:"
#else
#define CON_USAGE_DRIVER	""
#define CON_OPTSTR_DRIVER	""
#endif

#define CON_OPTSTR	"F:" CON_OPTSTR_MEM CON_OPTSTR_PCI CON_OPTSTR_DRIVER
#if defined(CONFIG_CON_MEM) || defined(CONFIG_CON_PCI) || defined(CONFIG_CON_DRIVER)
#define CON_USAGE	"{" CON_USAGE_FILE CON_USAGE_MEM CON_USAGE_PCI CON_USAGE_DRIVER "}"
#else
#define CON_USAGE	CON_USAGE_FILE
#endif

static const char *optstr = CON_OPTSTR "ht:v";

static int strptrcmp(const void *a, const void *b)
{
	return strcmp(*(char **)a, *(char **)b);
}

static void usage_eepmap_chips(const struct eepmap *eepmap)
{
	char const **stridx = NULL, **__stridx;
	int i, n, l;

	/* Build index of supported chips */
	for (n = 0, i = 0; i < ARRAY_SIZE(chips); ++i) {
		if (chips[i].eepmap != eepmap)
			continue;

		n++;
		__stridx = realloc(stridx, sizeof(stridx[0]) * n);
		if (!__stridx) {
			fprintf(stderr, "Unable to allocate index buffer for chip names\n");
			free(stridx);
			return;
		}
		stridx = __stridx;
		stridx[n - 1] = chips[i].name;
	}

	/* Sort chip names alphabetically */
	qsort(stridx, n, sizeof(stridx[0]), strptrcmp);

	/* Now we are ready to print supported chip names */
	l = printf("%18sSupported chip(s):", "");
	for (i = 0; i < n; ++i) {
		int _l = 1 + strlen(stridx[i]);
		int is_last = i + 1 == n;

		if (!is_last)
			_l += 1;	/* account comma symbol */
		l += _l;
		if (l > 80) {
			printf("\n");
			l = printf("%19s", "") + _l;
		}

		printf(" %s", stridx[i]);
		if (!is_last)
			printf(",");
	}
	printf("\n");

	free(stridx);
}

static void usage_eepmap(struct atheepmgr *aem, const struct eepmap *eepmap)
{
	const struct eepmap_param *param;
	bool raw_eep = eepmap->features & EEPMAP_F_RAW_EEP;
	bool raw_otp = eepmap->features & EEPMAP_F_RAW_OTP;
	char buf[0x100];
	int i;

	printf("  %-15s %s\n", eepmap->name, eepmap->desc);

	if (aem->verbose < 1)
		return;

	usage_eepmap_chips(eepmap);

	printf("%18sSupport for RAW contents saving: %s\n", "",
	       raw_eep || raw_otp ? "EEPROM, OTP" :
	       raw_eep ? "EEPROM" : raw_otp ? "OTP" : "none");
	printf("%18sSupport for unpacked data saving: %s\n", "",
	       eepmap->unpacked_buf_sz ? "Yes" : "No");
	printf("%18s%s:\n", "", "Supported sections for dumping");
	for (i = 0; i < EEP_SECT_MAX; ++i) {
		if (!eepmap->dump[i])
			continue;
		printf("%20s%-10s %s\n", "", eepmap_sections_list[i].name,
		       eepmap_sections_list[i].desc);
	}
	if (eepmap->params_mask && eepmap->update_eeprom) {
		printf("%18s%s:\n", "", "Updateable EEPROM params");
		for (param = &eepmap_params_list[0]; param->name; ++param) {
			if (!(eepmap->params_mask & BIT(param->id)))
				continue;

			if (param->arg)
				snprintf(buf, sizeof(buf), "%s=%s", param->name,
					 param->arg);
			else
				snprintf(buf, sizeof(buf), "%s", param->name);

			printf("%20s%-10s %s\n", "", buf, param->desc);
		}
	}
	if (eepmap->templates) {
		const struct eeptemplate *tpl;

		printf("%18sKnown EEPROM data templates:\n", "");
		for (tpl = eepmap->templates; tpl->name; ++tpl)
			printf("%20s%d: %s\n", "", tpl->id, tpl->name);
	}
}

static void usage(struct atheepmgr *aem, char *name)
{
	int i;

	printf(
		"Atheros NIC EEPROM management utility v2.1.1\n"
		"Copyright (c) 2008-2011, Atheros Communications Inc.\n"
		"Copyright (c) 2011-2012, Qualcomm Atheros, Inc.\n"
		"Copyright (c) 2013-2021, Sergey Ryazanov <ryazanov.s.a@gmail.com>\n"
		"\n"
		"Usage:\n"
		"  %s " CON_USAGE " [-t <eepmap>] [<action> [<actarg>]]\n"
		"or\n"
		"  %s -h\n"
		"\n"
		"Options:\n"
		"  -F <eepdump>    Read EEPROM dump from <eepdump> file.\n"
#if defined(CONFIG_CON_MEM)
		"  -M <ioaddr>     Interact with card via /dev/mem by mapping card I/O memory\n"
		"                  at <ioaddr> to the process.\n"
#endif
#if defined(CONFIG_CON_PCI)
		"  -P <slot>       Use libpciaccess to interact with card installed in <slot>.\n"
		"                  Slot should be specified in form:\n"
		"                  [<domain>:]<bus>:<device>[.<func>] as displayed by lspci(8)\n"
		"                  utility. If <domain> is omitted then domain 0 will be used.\n"
		"                  If <func> is omitted then first available function will be\n"
		"                  used.\n"
#endif
#if defined(CONFIG_CON_DRIVER)
		"  -D <dev>        Use driver debug interface to interact with <dev> card.\n"
#if defined(__linux__)
		"                  <dev> could be specified as a cfg80211 phy (e.g. phy0, phy1)\n"
		"                  or as a network device/interface (e.g. wlan0, wlan1)\n"
#endif
#endif
		"  -t <eepmap>     Override EEPROM map type (see below), this option is required\n"
		"                  for connectors, without PnP (map type autodetection) support.\n"
		"                  EEPROM map type could be specified by its name or by a name of\n"
		"                  chip.\n"
		"                  Also map type could be specified by a chip PCI Device ID,\n"
		"                  which should be prefixed with 'PCI:' string (e.g. 'PCI:0029'\n"
		"                  for AR9220 chip). Such method will be useful if you have no\n"
		"                  PCI connector, access chip via I/O memory connector and do not\n"
		"                  shure about an exact chip type. So you could check PCI Id with\n"
		"                  help of pciconf(8)/lspci(8)/pcidump(8) utility and then use\n"
		"                  obtained identifier to specify chip (and EEPROM map) type.\n"
		"  -v              Be verbose. I.e. print detailed help message, log action\n"
		"                  stages, print all EEPROM data including unused parameters.\n"
		"  -h              Print this cruft. Use -v option to see more details.\n"
		"  <action>        Optional argument, which specifies the <action> that should be\n"
		"                  performed (see actions list below). If no action is specified,\n"
		"                  then the 'dump' action is performed by default.\n"
		"  <actarg>        Action argument if the action accepts any (see details below\n"
		"                  in the detailed actions list).\n"
		"\n",
		name, name
	);

	if (aem->verbose) {
		printf(
			"Available actions:\n"
			"  dump [<sects>]  Read & parse the EEPROM content and then dump it to the\n"
			"                  terminal (this is the default action). An optional list of the\n"
			"                  comma-separated EEPROM sections <sect> can be specified as the\n"
			"                  argument. This argument tells the utility what sections should\n"
			"                  be dumped. See the list of awailable sections for dumping\n"
			"                  below in the EEPROM maps. Also there are two special keywords:\n"
			"                  'all' and 'none'. The first causes the dumping of all sections\n"
			"                  and the second disables any dumping to the terminal.\n"
			"                  The default action behaviour is to print the contents of all\n"
			"                  supported EEPROM sections.\n"
			"  save <file>     Save fetched raw EEPROM content to the file <file>.\n"
			"  saveraw <file>  Save the raw contents of the EEPROM or OTP mem without any\n"
			"                  pre-checks to the file <file>. This option is useful when the\n"
			"                  data is corrupted or the utility is unable to verify the data\n"
			"                  due to some internal issues.\n"
			"  saveraweep <file> Same as 'saveraw', but saves only the EEPROM contents\n"
			"                  without any other (e.g. OTP) memory types access.\n"
			"  saverawotp <file> Same as 'saveraw', but saves only the OTP mem contents\n"
			"                  without any other (e.g. EEPROM) memory types access.\n"
			"  unpack <file>   Save unpacked EEPROM/OTP data to the file <file>. Saved data\n"
			"                  type depends on EEPROM map type, usually only calibration\n"
			"                  data are saved.\n"
			"  update <param>[=<val>]  Set EEPROM parameter <param> to <val>. See per-map\n"
			"                  supported parameters list below.\n"
			"  templateexport <name-or-id> <file> Export template specified by Name or by Id\n"
			"                  to the file <file>.\n"
			"  gpiodump        Dump GPIO lines state to the terminal.\n"
			"  regread <addr>  Read register at address <addr> and print it value.\n"
			"  regwrite <addr> <val> Write value <val> to the register at address <addr>.\n"
			"\n"
		);
	} else {
		printf(
			"Available actions (use -v option to see details):\n"
			"  dump [<sects>]  Read & dump parsed EEPROM content to the terminal.\n"
			"  save <file>     Save fetched raw EEPROM content to the file <file>.\n"
			/* NB: 'saveraw' intentionally skipped to keep usage short. */
			"  unpack <file>   Save unpacked EEPROM/OTP calibration data to the file <file>.\n"
			"  update <param>[=<val>]  Set EEPROM parameter <param> to <val>.\n"
			/* NB: 'templateexport' intentionally skipped to keep usage short. */
			"  gpiodump        Dump GPIO lines state to the terminal.\n"
			"  regread <addr>  Read register at address <addr> and print it value.\n"
			"  regwrite <addr> <val> Write value <val> to the register at address <addr>.\n"
			"\n"
		);
	}

	printf(
		"Available connectors (card interactions interface):\n"
#if defined(CONFIG_CON_DRIVER)
		"  Driver          Interact with card via driver debuging interface, activated\n"
		"                  by -D option. Since each driver have a specific debug\n"
		"                  interface, utility first determine serving driver and then\n"
		"                  automatically configured themself to work with this driver.\n"
#endif
		"  File            Read EEPROM dump from file, activated by -F option with dump\n"
		"                  file path argument.\n"
#if defined(CONFIG_CON_MEM)
		"  Mem             Interact with card via /dev/mem by mapping device I/O memory\n"
		"                  to the proccess memory, activated by -M option with the device\n"
		"                  I/O memory region start address as the argument.\n"
#endif
#if defined(CONFIG_CON_PCI)
		"  PCI             Interact with card via libpciaccess library, activated by -P\n"
		"                  option with a device slot arg.\n"
#endif
		"\n"
	);

	if (aem->verbose)
		printf("Supported EEPROM map(s) and per-map capabilities:\n");
	else
		printf("Supported EEPROM map(s) and per-map capabilities (use -v option to see details):\n");
	for (i = 0; i < ARRAY_SIZE(eepmaps); ++i)
		usage_eepmap(aem, eepmaps[i]);
	printf("\n");
}

int main(int argc, char *argv[])
{
	struct atheepmgr *aem = &__aem;
	const struct action *act = NULL;
	const struct eepmap *user_eepmap = NULL;
	char *con_arg = NULL;
	int print_usage = 0;
	int i, opt;
	int ret;

	if (argc == 1)
		print_usage = 1;

	aem->host_is_be = __BYTE_ORDER == __BIG_ENDIAN;
	aem->eep_wp_gpio_num = EEP_WP_GPIO_AUTO;	/* Autodetection */
	aem->eep_wp_gpio_pol = 0;		/* Unlock by low level */

	ret = -EINVAL;
	while ((opt = getopt(argc, argv, optstr)) != -1) {
		switch (opt) {
		case 'F':
			aem->con = &con_file;
			con_arg = optarg;
			break;
#if defined(CONFIG_CON_MEM)
		case 'M':
			aem->con = &con_mem;
			con_arg = optarg;
			break;
#endif
#if defined(CONFIG_CON_PCI)
		case 'P':
			aem->con = &con_pci;
			con_arg = optarg;
			break;
#endif
#if defined(CONFIG_CON_DRIVER)
		case 'D':
			aem->con = &con_driver;
			con_arg = optarg;
			break;
#endif
		case 't':
			user_eepmap = eepmap_find_by_name(optarg);
			if (!user_eepmap)
				user_eepmap = eepmap_find_by_chip(optarg);
			if (!user_eepmap) {
				fprintf(stderr, "Unknown EEPROM map type or chip name or chip Id: %s\n",
					optarg);
				goto exit;
			}
			break;
		case 'v':
			aem->verbose++;
			break;
		case 'h':
			print_usage = 1;
			break;
		default:
			goto exit;
		}
	}

	if (print_usage) {
		usage(aem, argv[0]);
		ret = 0;
		goto exit;
	}

	if (optind >= argc) {
		act = &actions[0];
	} else {
		for (i = 0; i < ARRAY_SIZE(actions); ++i) {
			if (strcasecmp(argv[optind], actions[i].name) != 0)
				continue;
			act = &actions[i];
			break;
		}
		if (!act) {
			fprintf(stderr, "Unknown action -- %s\n", argv[optind]);
			goto exit;
		}
		optind++;
	}

	if (!aem->con) {
		if (act->flags & ACT_F_AUTONOMOUS) {
			aem->con = &con_stub;	/* to avoid conn. init crash */
		} else {
			fprintf(stderr, "Connector is not specified\n");
			goto exit;
		}
	}

	if ((act->flags & ACT_F_HW) && !(aem->con->caps & CON_CAP_HW)) {
		fprintf(stderr, "%s action require direct HW access, which is not proved by %s connector\n",
			act->name, aem->con->name);
		goto exit;
	}

	if (!user_eepmap && !(aem->con->caps & CON_CAP_PNP)) {
		fprintf(stderr, "EEPROM map type option is mandatory for connectors without chip autodetection (Plug and Play) support\n");
		goto exit;
	}

	aem->con_priv = malloc(aem->con->priv_data_sz);
	if (!aem->con_priv) {
		fprintf(stderr, "Unable to allocate memory for the connector private data\n");
		ret = -ENOMEM;
		goto exit;
	}

	ret = aem->con->init(aem, con_arg);
	if (ret)
		goto exit;

	if (!aem->eepmap && !user_eepmap) {
		fprintf(stderr, "Connector failed to autodetect EEPROM type, you need to specify it manually\n");
		goto con_clean;
	} else if (!aem->eepmap && user_eepmap) {
		if ((aem->con->caps & CON_CAP_PNP) && aem->verbose)
			printf("Connector failed to autodetect EEPROM type, use manually configured %s type\n",
			       user_eepmap->name);
		aem->eepmap = user_eepmap;
	} else if (aem->eepmap && !user_eepmap) {
		if (aem->verbose)
			printf("Autodetected EEPROM map type is %s\n",
			       aem->eepmap->name);
	} else if (aem->eepmap != user_eepmap) {
		if (aem->verbose)
			printf("Override autodetected %s EEPROM type with manually configured %s type\n",
			       aem->eepmap->name, user_eepmap->name);
		aem->eepmap = user_eepmap;
	}

	if (act->flags & ACT_F_RAW_DATA) {
		ret = -EINVAL;
		if ((act->flags & ACT_F_RAW_DATA) == ACT_F_RAW_EEP &&
		    !(aem->eepmap->features & EEPMAP_F_RAW_EEP))
			fprintf(stderr, "EEPROM map does not support RAW EEPROM contents loading\n");
		else if ((act->flags & ACT_F_RAW_DATA) == ACT_F_RAW_OTP &&
			   !(aem->eepmap->features & EEPMAP_F_RAW_OTP))
			fprintf(stderr, "EEPROM map does not support RAW OTP contents loading\n");
		else if (!(aem->eepmap->features & EEPMAP_F_RAW_DATA))
			fprintf(stderr, "EEPROM map does not support any RAW data loading\n");
		else
			ret = 0;
		if (ret)
			goto con_clean;
	}

	if (aem->con->caps & CON_CAP_HW) {
		ret = hw_init(aem);
		if (ret)
			goto con_clean;

		if (aem->eep_wp_gpio_num != EEP_WP_GPIO_NONE &&
		    aem->eep_wp_gpio_num >= aem->gpio_num) {
			fprintf(stderr, "EEPROM unlocking GPIO #%d is out of range 0...%d\n",
				aem->eep_wp_gpio_num, aem->gpio_num - 1);
			goto con_clean;
		}
	}

	if (act->flags & ACT_F_DATA) {
		int tries = 0;

		hw_eeprom_set_ops(aem);
		hw_otp_set_ops(aem);

		aem->eepmap_priv = malloc(aem->eepmap->priv_data_sz);
		if (!aem->eepmap_priv) {
			fprintf(stderr, "Unable to allocate memory for the EEPROM parser private data\n");
			ret = -ENOMEM;
			goto con_clean;
		}

		aem->eep_buf = malloc(aem->eepmap->eep_buf_sz *
				      sizeof(uint16_t));
		if (!aem->eep_buf) {
			fprintf(stderr, "Unable to allocate memory for EEPROM buffer\n");
			ret = -ENOMEM;
			goto con_clean;
		}
		if (aem->eepmap->unpacked_buf_sz) {
			aem->unpacked_buf =
					malloc(aem->eepmap->unpacked_buf_sz);
			if (!aem->unpacked_buf) {
				fprintf(stderr, "Unable to allocate memory for buffer of unpacked data\n");
				ret = -ENOMEM;
				goto con_clean;
			}
		}

		if (act->flags & ACT_F_RAW_EEP &&
		    aem->eepmap->features & EEPMAP_F_RAW_EEP &&
		    aem->eep && aem->eepmap->load_eeprom) {
			tries++;
			if (aem->verbose > 1)
				printf("Try to load RAW EEPROM data\n");
			if (aem->eepmap->load_eeprom(aem, true))
				goto loading_done;
		}
		if (act->flags & ACT_F_RAW_OTP &&
		    aem->eepmap->features & EEPMAP_F_RAW_OTP &&
		    aem->otp && aem->eepmap->load_otp) {
			tries++;
			if (aem->verbose > 1)
				printf("Try to load RAW OTP data\n");
			if (aem->eepmap->load_otp(aem, true))
				goto loading_done;
		}
		if (act->flags & ACT_F_RAW_DATA)
			goto no_data;

		if (aem->con->blob && aem->eepmap->load_blob) {
			tries++;
			if (aem->verbose > 1)
				printf("Try to load data from blob\n");
			if (aem->eepmap->load_blob(aem))
				goto loading_done;
		}
		if (aem->eep && aem->eepmap->load_eeprom) {
			tries++;
			if (aem->verbose > 1)
				printf("Try to load data from EEPROM\n");
			if (aem->eepmap->load_eeprom(aem, false))
				goto loading_done;
		}
		if (aem->otp && aem->eepmap->load_otp) {
			tries++;
			if (aem->verbose > 1)
				printf("Try to load data from OTP memory\n");
			if (aem->eepmap->load_otp(aem, false))
				goto loading_done;
		}

no_data:
		if (tries) {
			fprintf(stderr, "Unable to load data from any sources\n");
			ret = -EIO;
		} else {
			fprintf(stderr, "No suitable data source in available via configured connector\n");
			ret = -EINVAL;
		}
		goto con_clean;

loading_done:
		if (!(act->flags & ACT_F_RAW_DATA) &&
		    !aem->eepmap->check_eeprom(aem)) {
			fprintf(stderr, "EEPROM check failed\n");
			ret = -EINVAL;
			goto con_clean;
		}
	}

	ret = act->func(aem, argc - optind, argv + optind);

con_clean:
	aem->con->clean(aem);

exit:
	free(aem->unpacked_buf);
	free(aem->eep_buf);
	free(aem->eepmap_priv);
	free(aem->con_priv);

	return ret;
}
