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

int dump;

static struct edump __edump;

static const struct eepmap * const eepmaps[] = {
	&eepmap_def,
	&eepmap_4k,
	&eepmap_9287,
	&eepmap_9003,
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
		edump->eepmap = &eepmap_9003;
	} else if (AR_SREV_9287(edump)) {
		edump->eepmap = &eepmap_9287;
	} else if (AR_SREV_9285(edump)) {
		edump->eepmap = &eepmap_4k;
	} else {
		edump->eepmap = &eepmap_def;
	}

	printf("Detected EEPROM map: %s\n", edump->eepmap->name);

	return 0;
}

void dump_device(struct edump *edump)
{
	if (register_eepmap(edump) < 0)
		return;

	if (!edump->eepmap->fill_eeprom(edump)) {
		fprintf(stderr, "Unable to fill EEPROM data\n");
		return;
	}

	if (!edump->eepmap->check_eeprom(edump)) {
		fprintf(stderr, "EEPROM check failed\n");
		return;
	}

	switch(dump) {
	case DUMP_BASE_HEADER:
		edump->eepmap->dump_base_header(edump);
		break;
	case DUMP_MODAL_HEADER:
		edump->eepmap->dump_modal_header(edump);
		break;
	case DUMP_POWER_INFO:
		edump->eepmap->dump_power_info(edump);
		break;
	case DUMP_ALL:
		edump->eepmap->dump_base_header(edump);
		edump->eepmap->dump_modal_header(edump);
		edump->eepmap->dump_power_info(edump);
		break;
	}
}

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

static const char *optstr = CON_OPTSTR "ambpht:";

static void usage(char *name)
{
	int i;

	printf(
		"Atheros NIC EEPROM dump utility.\n"
		"\n"
		"Usage:\n"
		"  %s " CON_USAGE " [-t <eepmap>] [-bmpa]\n"
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
		"  -b              Dump base EEPROM header.\n"
		"  -m              Dump modal EEPROM header(s).\n"
		"  -p              Dump power calibration EEPROM info.\n"
		"  -a              Dump everything from EEPROM (default).\n"
		"  -t <eepmap>     Override EEPROM map type (see below), this option is required\n"
		"                  for connectors, without direct HW access.\n"
		"  -h              Print this cruft.\n"
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
	char *con_arg = NULL;
	int opt;
	int ret;

	if (argc == 1) {
		usage(argv[0]);
		return 0;
	}

	dump = DUMP_ALL;

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
		case 'b':
			dump = DUMP_BASE_HEADER;
			break;
		case 'm':
			dump = DUMP_MODAL_HEADER;
			break;
		case 'p':
			dump = DUMP_POWER_INFO;
			break;
		case 'a':
			dump = DUMP_ALL;
			break;
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
	if (!edump->eepmap && !(edump->con->caps & CON_CAP_HW)) {
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

	dump_device(edump);

	edump->con->clean(edump);

exit:
	free(edump->con_priv);

	return ret;
}
