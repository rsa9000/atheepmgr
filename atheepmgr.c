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
#include "utils.h"

static struct atheepmgr __aem;

static const struct eepmap * const eepmaps[] = {
	&eepmap_5211,
	&eepmap_5416,
	&eepmap_9285,
	&eepmap_9287,
	&eepmap_9300,
};

static const struct eepmap *eepmap_find_by_name(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(eepmaps); ++i) {
		if (strcasecmp(eepmaps[i]->name, name) == 0)
			return eepmaps[i];
	}

	return NULL;
}

static int eepmap_detect(struct atheepmgr *aem)
{
	if (AR_SREV_9300_20_OR_LATER(aem)) {
		aem->eepmap = &eepmap_9300;
	} else if (AR_SREV_9287(aem)) {
		aem->eepmap = &eepmap_9287;
	} else if (AR_SREV_9285(aem)) {
		aem->eepmap = &eepmap_9285;
	} else if (AR_SREV_5416_OR_LATER(aem)) {
		aem->eepmap = &eepmap_5416;
	} else if (AR_SREV_5211_OR_LATER(aem)) {
		aem->eepmap = &eepmap_5211;
	} else {
		fprintf(stderr, "Unable to determine an EEPROM map suitable for this chip\n");
		return -ENOENT;
	}

	printf("Detected EEPROM map: %s\n", aem->eepmap->name);

	return 0;
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

#define ACT_F_EEPROM	(1 << 0)	/* Action will interact with EEPROM */
#define ACT_F_HW	(1 << 1)	/* Action require direct HW access */

static const struct action {
	const char *name;
	int (*func)(struct atheepmgr *aem, int argc, char *argv[]);
	int flags;
} actions[] = {
	{
		.name = "dump",
		.func = act_eep_dump,
		.flags = ACT_F_EEPROM,
	}, {
		.name = "save",
		.func = act_eep_save,
		.flags = ACT_F_EEPROM,
	}, {
		.name = "update",
		.func = act_eep_update,
		.flags = ACT_F_EEPROM,
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

#if defined(CONFIG_CON_PCI) && defined(CONFIG_CON_MEM)
#define CON_OPTSTR	"F:M:P:"
#define CON_USAGE	"{-F <eepdump> | -M <ioaddr> | -P <slot>}"
#elif defined(CONFIG_CON_MEM)
#define CON_OPTSTR	"F:M:"
#define CON_USAGE	"{-F <eepdump> | -M <ioaddr>}"
#elif defined(CONFIG_CON_PCI)
#define CON_OPTSTR	"F:P:"
#define CON_USAGE	"{-F <eepdump> | -P <slot>}"
#else
#define CON_OPTSTR	"F:"
#define CON_USAGE	"-F <eepdump>"
#endif

static const char *optstr = CON_OPTSTR "ht:";

static void usage_eepmap(const struct eepmap *eepmap)
{
	const struct eepmap_param *param;
	char buf[0x100];
	int i;

	printf("  %-15s %s\n", eepmap->name, eepmap->desc);
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
}

static void usage(char *name)
{
	int i;

	printf(
		"Atheros NIC EEPROM management utility.\n"
		"\n"
		"Usage:\n"
		"  %s " CON_USAGE " [-t <eepmap>] [<action> [<actarg>]]\n"
		"or\n"
		"  %s -h\n"
		"\n"
		"Options:\n"
		"  -F <eepdump>    Read EEPROM dump from <eepdump> file.\n"
#if defined(CONFIG_CON_MEM)
		"  -M <ioaddr>     Interact with card via /dev/mem by mapping\n"
		"                  card I/O memory at <ioaddr> to the process.\n"
#endif
#if defined(CONFIG_CON_PCI)
		"  -P <slot>       Use libpciaccess to interact with card installed\n"
		"                  in <slot>. Slot should be specified in form:\n"
		"                  [<domain>:]<bus>:<device>[.<func>] as displayed\n"
		"                  by lspci(8) utility. If <domain> is omitted\n"
		"                  then domain 0 will be used. If <func> is omitted\n"
		"                  then first available function will be used.\n"
#endif
		"  -t <eepmap>     Override EEPROM map type (see below), this option is required\n"
		"                  for connectors, without direct HW access.\n"
		"  -h              Print this cruft.\n"
		"  <action>        Optional argument, which specifies the <action> that should be\n"
		"                  performed (see actions list below). If no action is specified,\n"
		"                  the 'dump' action is performed by default.\n"
		"  <actarg>        Action argument if the action accepts any (see details below\n"
		"                  in the detailed actions list).\n"
		"\n"
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
		"  update <param>[=<val>]  Set EEPROM parameter <param> to <val>. See per-map\n"
		"                  supported parameters list below.\n"
		"  gpiodump        Dump GPIO lines state to the terminal.\n"
		"  regread <addr>  Read register at address <addr> and print it value.\n"
		"  regwrite <addr> <val> Write value <val> to the register at address <addr>.\n"
		"\n"
		"Available connectors (card interactions interface):\n"
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
		"\n",
		name, name
	);

	printf("Supported EEPROM map(s) and per-map capabilities:\n");
	for (i = 0; i < ARRAY_SIZE(eepmaps); ++i)
		usage_eepmap(eepmaps[i]);
	printf("\n");
}

int main(int argc, char *argv[])
{
	struct atheepmgr *aem = &__aem;
	const struct action *act = NULL;
	char *con_arg = NULL;
	int i, opt;
	int ret;

	if (argc == 1) {
		usage(argv[0]);
		return 0;
	}

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
		case 't':
			aem->eepmap = eepmap_find_by_name(optarg);
			if (!aem->eepmap) {
				fprintf(stderr, "Unknown EEPROM map type name: %s\n",
					optarg);
				goto exit;
			}
			break;
		case 'h':
			usage(argv[0]);
			ret = 0;
			goto exit;
		default:
			goto exit;
		}
	}

	if (!aem->con) {
		fprintf(stderr, "Connector is not specified\n");
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

	if ((act->flags & ACT_F_HW) && !(aem->con->caps & CON_CAP_HW)) {
		fprintf(stderr, "%s action require direct HW access, which is not proved by %s connector\n",
			act->name, aem->con->name);
		goto exit;
	}

	if ((act->flags & ACT_F_EEPROM) && !aem->eepmap &&
	    !(aem->con->caps & CON_CAP_HW)) {
		fprintf(stderr, "EEPROM map type option is mandatory for connectors without direct HW access\n");
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

	if (act->flags & ACT_F_EEPROM) {
		hw_eeprom_set_ops(aem);

		if (!aem->eepmap) {
			ret = eepmap_detect(aem);
			if (ret)
				goto con_clean;
		}

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

		if (!aem->eepmap->fill_eeprom(aem)) {
			fprintf(stderr, "Unable to fill EEPROM data\n");
			ret = -EIO;
			goto con_clean;
		}

		if (!aem->eepmap->check_eeprom(aem)) {
			fprintf(stderr, "EEPROM check failed\n");
			ret = -EINVAL;
			goto con_clean;
		}
	}

	ret = act->func(aem, argc - optind, argv + optind);

con_clean:
	aem->con->clean(aem);

exit:
	free(aem->eep_buf);
	free(aem->eepmap_priv);
	free(aem->con_priv);

	return ret;
}
