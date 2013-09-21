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

static const char *
mac_bb_name(uint32_t mac_bb_version)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mac_bb_names); i++) {
		if (mac_bb_names[i].version == mac_bb_version) {
			return mac_bb_names[i].name;
		}
	}

	return "????";
}

static int is_supported_chipset(struct pci_device *pdev)
{
	if (pdev->vendor_id != ATHEROS_VENDOR_ID)
		return 0;

	if ((pdev->device_id != AR5416_DEVID_PCI) &&
	    (pdev->device_id != AR5416_DEVID_PCIE) &&
	    (pdev->device_id != AR9160_DEVID_PCI) &&
	    (pdev->device_id != AR9280_DEVID_PCI) &&
	    (pdev->device_id != AR9280_DEVID_PCIE) &&
	    (pdev->device_id != AR9285_DEVID_PCIE) &&
	    (pdev->device_id != AR9287_DEVID_PCI) &&
	    (pdev->device_id != AR9287_DEVID_PCIE) &&
	    (pdev->device_id != AR9300_DEVID_PCIE) &&
	    (pdev->device_id != AR9485_DEVID_PCIE) &&
	    (pdev->device_id != AR9580_DEVID_PCIE) &&
	    (pdev->device_id != AR9462_DEVID_PCIE) &&
	    (pdev->device_id != AR9565_DEVID_PCIE) &&
	    (pdev->device_id != AR1111_DEVID_PCIE)) {
		fprintf(stderr, "Device ID: 0x%x not supported\n", pdev->device_id);
		return 0;
	}

	printf("Found Device ID: 0x%04x\n", pdev->device_id);
	return 1;
}

static int init_pci_device(struct edump *edump, struct pci_device *pdev)
{
	int err;

	if (!pdev->regions[0].base_addr) {
		fprintf(stderr, "Invalid base address\n");
		return EINVAL;
	}

	edump->pdev = pdev;
	edump->base_addr = pdev->regions[0].base_addr;
	edump->size = pdev->regions[0].size;
	pdev->user_data = (intptr_t)edump;

	if ((err = pci_device_map_range(pdev, edump->base_addr, edump->size,
					0, &edump->io_map)) != 0) {
		fprintf(stderr, "%s\n", strerror(err));
		return err;
	}

	printf("Mapped IO region at: %p\n", edump->io_map);

	return 0;
}

static void cleanup_pci_device(struct edump *edump)
{
	int err;

	printf("\nFreeing Mapped IO region at: %p\n", edump->io_map);

	if ((err = pci_device_unmap_range(edump->pdev, edump->io_map,
					  edump->size)) != 0)
		fprintf(stderr, "%s\n", strerror(err));
}

static void hw_read_revisions(struct edump *edump)
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

bool pci_eeprom_read(struct edump *edump, uint32_t off, uint16_t *data)
{
	(void)REG_READ(AR5416_EEPROM_OFFSET + (off << AR5416_EEPROM_S));

	if (!hw_wait(edump,
		     AR_EEPROM_STATUS_DATA,
		     AR_EEPROM_STATUS_DATA_BUSY |
		     AR_EEPROM_STATUS_DATA_PROT_ACCESS, 0,
		     AH_WAIT_TIMEOUT)) {
		return false;
	}

	*data = MS(REG_READ(AR_EEPROM_STATUS_DATA),
		   AR_EEPROM_STATUS_DATA_VAL);

	return true;
}

int register_eep_ops(struct edump *edump)
{
	if (AR_SREV_9300_20_OR_LATER(edump)) {
		edump->eep_map = EEP_MAP_9003;
		edump->eep_ops = &eep_9003_ops;
	} else if (AR_SREV_9287(edump)) {
		edump->eep_map = EEP_MAP_9287;
		edump->eep_ops = &eep_9287_ops;
	} else if (AR_SREV_9285(edump)) {
		edump->eep_map = EEP_MAP_4K;
		edump->eep_ops = &eep_4k_ops;
	} else {
		edump->eep_map = EEP_MAP_DEFAULT;
		edump->eep_ops = &eep_def_ops;
	}

	if (!edump->eep_ops->fill_eeprom(edump)) {
		fprintf(stderr, "Unable to fill EEPROM data\n");
		return -1;
	}

	if (!edump->eep_ops->check_eeprom(edump)) {
		fprintf(stderr, "EEPROM check failed\n");
		return -1;
	}

	return 0;
}

void dump_device(struct edump *edump)
{
	hw_read_revisions(edump);

	if (register_eep_ops(edump) < 0)
		return;

	switch(dump) {
	case DUMP_BASE_HEADER:
		edump->eep_ops->dump_base_header(edump);
		break;
	case DUMP_MODAL_HEADER:
		edump->eep_ops->dump_modal_header(edump);
		break;
	case DUMP_POWER_INFO:
		edump->eep_ops->dump_power_info(edump);
		break;
	case DUMP_ALL:
		edump->eep_ops->dump_base_header(edump);
		edump->eep_ops->dump_modal_header(edump);
		edump->eep_ops->dump_power_info(edump);
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
	char *pci_slot_str = NULL;
	struct pci_slot_match slot[2];
	struct pci_device_iterator *iter;
	struct pci_device *pdev;
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
			pci_slot_str = optarg;
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

	if (!pci_slot_str) {
		fprintf(stderr, "PCI device slot is not specified\n");
		return -EINVAL;
	}

	memset(slot, 0x00, sizeof(slot));

	ret = sscanf(pci_slot_str, "%x:%x:%x", &slot[0].domain, &slot[0].bus, &slot[0].dev);
	if (ret != 3) {
		fprintf(stderr, "Invalid PCI slot specification: %s\n", pci_slot_str);
		ret = -EINVAL;
		goto exit;
	}
	slot[0].func = PCI_MATCH_ANY;

	if ((ret = pci_system_init()) != 0) {
		fprintf(stderr, "%s\n", strerror(ret));
		goto exit;
	} else {
		printf("Initializing PCI\n");
	}

	iter = pci_slot_match_iterator_create(slot);
	if (iter == NULL) {
		ret = -EINVAL;
		fprintf(stderr, "Iter creation failed\n");
		goto err;
	}

	pdev = pci_device_next(iter);
	pci_iterator_destroy(iter);

	if (NULL == pdev) {
		ret = -ENODEV;
		fprintf(stderr, "No suitable card found\n");
		goto err;
	}

	if ((ret = pci_device_probe(pdev)) != 0) {
		fprintf(stderr, "%s\n", strerror(ret));
		goto err;
	}

	if (!is_supported_chipset(pdev)) {
		ret = -ENOTSUP;
		goto err;
	}

	ret = init_pci_device(edump, pdev);
	if (ret)
		goto err;

	dump_device(edump);
	cleanup_pci_device(edump);

err:
	pci_system_cleanup();

exit:
	return ret;
}
