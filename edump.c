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

static struct edump __edump;

static const struct eepmap * const eepmaps[] = {
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

int register_eepmap(struct edump *edump)
{
	if (edump->eepmap)
		return 0;

	if (AR_SREV_9300_20_OR_LATER(edump)) {
		edump->eepmap = &eepmap_9300;
	} else if (AR_SREV_9287(edump)) {
		edump->eepmap = &eepmap_9287;
	} else if (AR_SREV_9285(edump)) {
		edump->eepmap = &eepmap_9285;
	} else {
		edump->eepmap = &eepmap_5416;
	}

	printf("Detected EEPROM map: %s\n", edump->eepmap->name);

	return 0;
}

static int act_eep_dump(struct edump *edump, int argc, char *argv[])
{
	struct {
		const char *name;
		void (*func)(struct edump *edump);
	} known_sections[] = {
		{"base", edump->eepmap->dump_base_header},
		{"modal", edump->eepmap->dump_modal_header},
		{"power", edump->eepmap->dump_power_info},
	};
	static char def[4] = {'a', 'l', 'l', '\0'};
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

		for (i = 0; i < ARRAY_SIZE(known_sections); ++i)
			if (strcasecmp(tok, known_sections[i].name) == 0)
				break;
		if (i == ARRAY_SIZE(known_sections)) {
			fprintf(stderr, "Unknown EEPROM section to dump -- %s\n",
				tok);
			return -EINVAL;
		}

		dump_mask |= 1 << i;
	}

	for (i = 0; i < ARRAY_SIZE(known_sections); ++i) {
		if (!(dump_mask & (1 << i)))
			continue;

		known_sections[i].func(edump);
	}

	return 0;
}

static int act_eep_save(struct edump *edump, int argc, char *argv[])
{
	FILE *fp;
	const uint16_t *buf = edump->eep_buf;
	int buf_sz = edump->eepmap->eep_buf_sz;
	size_t res;

	if (!buf_sz) {
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

	res = fwrite(buf, sizeof(buf[0]), buf_sz, fp);
	if (res != buf_sz)
		fprintf(stderr, "Unable to save whole EEPROM contents: %s\n",
			strerror(errno));

	fclose(fp);

	return res == buf_sz ? 0 : -EIO;
}

static int act_reg_read(struct edump *edump, int argc, char *argv[])
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

static int act_reg_write(struct edump *edump, int argc, char *argv[])
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
	int (*func)(struct edump *edump, int argc, char *argv[]);
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

static void usage(char *name)
{
	int i;

	printf(
		"Atheros NIC EEPROM dump utility.\n"
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
		"                  in <slot>. Slot consist of 3 parts devided by colon:\n"
		"                  <slot> = <domain>:<bus>:<dev> as displayed by lspci.\n"
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
		"                  terminal (default action). An optional list of the\n"
		"                  comma-separated EEPROM sections <sect> can be specified as the\n"
		"                  action argument. This argument tells the utility what sections\n"
		"                  should be dumped. The following sections are supported:\n"
		"                  'base', 'modal' and 'power'. These are for: dump of the base\n"
		"                  EEPROM header, dump of the modal EEPROM header(s) and dump of\n"
		"                  the power information respectively. Also there are two special\n"
		"                  keywords: 'all' and 'none'. The first causes the dump of all\n"
		"                  sections and the second disables any dump to the terminal. The\n"
		"                  default behaviour is to dump all EEPROM sections.\n"
		"  save <file>     Save fetched raw EEPROM content to the file <file>.\n"
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

	printf("Supported EEPROM map(s):\n");
	for (i = 0; i < ARRAY_SIZE(eepmaps); ++i)
		printf("  %-15s %s\n", eepmaps[i]->name, eepmaps[i]->desc);
	printf("\n");
}

int main(int argc, char *argv[])
{
	struct edump *edump = &__edump;
	const struct action *act = NULL;
	char *con_arg = NULL;
	int i, opt;
	int ret;

	if (argc == 1) {
		usage(argv[0]);
		return 0;
	}

	edump->host_is_be = __BYTE_ORDER == __BIG_ENDIAN;

	ret = -EINVAL;
	while ((opt = getopt(argc, argv, optstr)) != -1) {
		switch (opt) {
		case 'F':
			edump->con = &con_file;
			con_arg = optarg;
			break;
#if defined(CONFIG_CON_MEM)
		case 'M':
			edump->con = &con_mem;
			con_arg = optarg;
			break;
#endif
#if defined(CONFIG_CON_PCI)
		case 'P':
			edump->con = &con_pci;
			con_arg = optarg;
			break;
#endif
		case 't':
			edump->eepmap = eepmap_find_by_name(optarg);
			if (!edump->eepmap) {
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

	if (!edump->con) {
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

	if ((act->flags & ACT_F_HW) && !(edump->con->caps & CON_CAP_HW)) {
		fprintf(stderr, "%s action require direct HW access, which is not proved by %s connector\n",
			act->name, edump->con->name);
		goto exit;
	}

	if ((act->flags & ACT_F_EEPROM) && !edump->eepmap &&
	    !(edump->con->caps & CON_CAP_HW)) {
		fprintf(stderr, "EEPROM map type option is mandatory for connectors without direct HW access\n");
		goto exit;
	}

	edump->con_priv = malloc(edump->con->priv_data_sz);
	if (!edump->con_priv) {
		fprintf(stderr, "Unable to allocate memory for the connector private data\n");
		ret = -ENOMEM;
		goto exit;
	}

	ret = edump->con->init(edump, con_arg);
	if (ret)
		goto exit;

	if (edump->con->caps & CON_CAP_HW)
		hw_read_revisions(edump);

	if (act->flags & ACT_F_EEPROM) {
		ret = register_eepmap(edump);
		if (ret)
			goto con_clean;

		edump->eepmap_priv = malloc(edump->eepmap->priv_data_sz);
		if (!edump->eepmap_priv) {
			fprintf(stderr, "Unable to allocate memory for the EEPROM parser private data\n");
			ret = -ENOMEM;
			goto con_clean;
		}

		edump->eep_buf = malloc(edump->eepmap->eep_buf_sz *
					sizeof(uint16_t));
		if (!edump->eep_buf) {
			fprintf(stderr, "Unable to allocate memory for EEPROM buffer\n");
			ret = -ENOMEM;
			goto con_clean;
		}

		if (!edump->eepmap->fill_eeprom(edump)) {
			fprintf(stderr, "Unable to fill EEPROM data\n");
			ret = -EIO;
			goto con_clean;
		}

		if (!edump->eepmap->check_eeprom(edump)) {
			fprintf(stderr, "EEPROM check failed\n");
			ret = -EINVAL;
			goto con_clean;
		}
	}

	ret = act->func(edump, argc - optind, argv + optind);

con_clean:
	edump->con->clean(edump);

exit:
	free(edump->eep_buf);
	free(edump->eepmap_priv);
	free(edump->con_priv);

	return ret;
}
