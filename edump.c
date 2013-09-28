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

int register_eepmap(struct edump *edump)
{
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

static const char *optstr = "P:ambph";

static void usage(char *name)
{
	printf(
		"Atheros NIC EEPROM dump utility.\n"
		"\n"
		"Usage:\n"
		"  %s -P <slot> [-bmpa]\n"
		"or\n"
		"  %s -h\n"
		"\n"
		"Options:\n"
		"  -P <slot>       Use libpciaccess to interact with card installed\n"
		"                  in <slot>. Slot consist of 3 parts devided by colon:\n"
		"                  <slot> = <domain>:<bus>:<dev> as displayed by lspci.\n"
		"  -b              Dump base EEPROM header.\n"
		"  -m              Dump modal EEPROM header(s).\n"
		"  -p              Dump power calibration EEPROM info.\n"
		"  -a              Dump everything from EEPROM (default).\n"
		"  -h              Print this cruft.\n"
		"\n",
		name, name
	);
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
		case 'P':
			edump->con = &con_pci;
			con_arg = optarg;
			break;
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
