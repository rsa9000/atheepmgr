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

#include <fcntl.h>
#include <pciaccess.h>

#include "atheepmgr.h"
#include "con_pci.h"

struct pci_priv {
#if defined(__OpenBSD__)
	int memfd;
#endif
	struct pci_device *pdev;
	pciaddr_t base_addr;
	pciaddr_t size;
	void *io_map;
};

static int is_supported_chipset(struct atheepmgr *aem, struct pci_device *pdev)
{
	static const struct {
		uint16_t dev_id;
		const char *name;
	} devs[] = {
		{AR5211_DEVID_PCI,  "AR5211 PCI"},
		{AR5212_DEVID_PCI,  "AR5212/AR5213 PCI"},
		{AR5413_DEVID_PCI,  "AR5413/AR5414 PCI"},
		{AR5416_DEVID_PCI,  "AR5416 PCI"},
		{AR5416_DEVID_PCIE, "AR5416 PCIe"},
		{AR9160_DEVID_PCI,  "AR9160 PCI"},
		{AR9280_DEVID_PCI,  "AR9280 PCI"},
		{AR9280_DEVID_PCIE, "AR9280 PCIe"},
		{AR9285_DEVID_PCIE, "AR9285 PCIe"},
		{AR9287_DEVID_PCI,  "AR9287 PCI"},
		{AR9287_DEVID_PCIE, "AR9287 PCIe"},
		{AR9300_DEVID_PCIE, "AR9300 PCIe"},
		{AR9485_DEVID_PCIE, "AR9485 PCIe"},
		{AR9580_DEVID_PCIE, "AR9580 PCIe"},
		{AR9462_DEVID_PCIE, "AR9462 PCIe"},
		{AR9565_DEVID_PCIE, "AR9565 PCIe"},
		{AR1111_DEVID_PCIE, "AR1111 PCIe"},
	};
	int i;

	if (pdev->vendor_id != ATHEROS_VENDOR_ID)
		goto not_supported;

	for (i = 0; i < ARRAY_SIZE(devs); ++i)
		if (devs[i].dev_id == pdev->device_id)
			break;

	if (i == ARRAY_SIZE(devs))
		goto not_supported;

	if (aem->verbose)
		printf("Found Device: %04x:%04x (%s)\n", pdev->vendor_id,
			pdev->device_id, devs[i].name);

	return 1;

not_supported:
	fprintf(stderr, "Device: %04x:%04x not supported\n",
		pdev->vendor_id, pdev->device_id);

	return 0;
}

static int pci_device_init(struct atheepmgr *aem, struct pci_device *pdev)
{
	struct pci_priv *ppd = aem->con_priv;
	int err;

	if (!pdev->regions[0].base_addr) {
		fprintf(stderr, "Invalid base address\n");
		return EINVAL;
	}

#if defined(__OpenBSD__)
	ppd->memfd = open("/dev/mem", O_RDWR);
	if (ppd->memfd < 0) {
		fprintf(stderr, "Opening /dev/mem failed: %s\n", strerror(errno));
		return errno;
	}
	pci_system_init_dev_mem(ppd->memfd);
#endif

	ppd->pdev = pdev;
	ppd->base_addr = pdev->regions[0].base_addr;
	ppd->size = pdev->regions[0].size;
	pdev->user_data = (intptr_t)aem;

	if (aem->verbose)
		printf("Try to map %08lx-%08lx I/O region to the process memory\n",
		       (unsigned long)ppd->base_addr,
		       (unsigned long)(ppd->base_addr + ppd->size - 1));

	err = pci_device_map_range(pdev, ppd->base_addr, ppd->size,
				   PCI_DEV_MAP_FLAG_WRITABLE, &ppd->io_map);
	if (err) {
		fprintf(stderr, "Unable to map mem range: %s (%d)\n", strerror(err), err);
		return err;
	}

	if (aem->verbose)
		printf("Mapped IO region at: %p\n", ppd->io_map);

	return 0;
}

static void pci_device_cleanup(struct atheepmgr *aem)
{
	struct pci_priv *ppd = aem->con_priv;
	int err;

	if (aem->verbose)
		printf("Freeing Mapped IO region at: %p\n", ppd->io_map);

	err = pci_device_unmap_range(ppd->pdev, ppd->io_map, ppd->size);
	if (err)
		fprintf(stderr, "%s\n", strerror(err));
}

static uint32_t pci_reg_read(struct atheepmgr *aem, uint32_t reg)
{
	struct pci_priv *ppd = aem->con_priv;

	return *((volatile uint32_t *)(ppd->io_map + reg));
}

static void pci_reg_write(struct atheepmgr *aem, uint32_t reg, uint32_t val)
{
	struct pci_priv *ppd = aem->con_priv;

	*((volatile uint32_t *)(ppd->io_map + reg)) = val;
}

static void pci_reg_rmw(struct atheepmgr *aem, uint32_t reg, uint32_t set,
			uint32_t clr)
{
	struct pci_priv *ppd = aem->con_priv;
	uint32_t tmp;

	tmp = *((volatile uint32_t *)(ppd->io_map + reg));
	tmp &= ~clr;
	tmp |= set;
	*((volatile uint32_t *)(ppd->io_map + reg)) = tmp;
}

static int pci_parse_devarg(const char *str, struct pci_slot_match *slot)
{
	int num, len;

	num = sscanf(str, "%x:%x:%x.%u%n", &slot->domain, &slot->bus,
		     &slot->dev, &slot->func, &len);
	if (num == 4 && str[len] == '\0')
		return 0;

	num = sscanf(str, "%x:%x:%x%n", &slot->domain, &slot->bus, &slot->dev,
		     &len);
	if (num == 3 && str[len] == '\0') {
		slot->func = PCI_MATCH_ANY;
		return 0;
	}

	num = sscanf(str, "%x:%x.%u%n", &slot->bus, &slot->dev, &slot->func,
		     &len);
	if (num == 3 && str[len] == '\0') {
		slot->domain = 0;
		return 0;
	}

	num = sscanf(str, "%x:%x%n", &slot->bus, &slot->dev, &len);
	if (num == 2 && str[len] == '\0') {
		slot->domain = 0;
		slot->func = PCI_MATCH_ANY;
		return 0;
	}

	return -1;
}

static int pci_init(struct atheepmgr *aem, const char *arg_str)
{
	struct pci_slot_match slot[2];
	struct pci_device_iterator *iter;
	struct pci_device *pdev;
	int ret;

	memset(slot, 0x00, sizeof(slot));

	ret = pci_parse_devarg(arg_str, &slot[0]);
	if (ret != 0) {
		fprintf(stderr, "Invalid PCI slot specification -- %s\n",
			arg_str);
		return -EINVAL;
	}

	ret = pci_system_init();
	if (ret) {
		fprintf(stderr, "PCI sys init error: %s\n", strerror(ret));
		goto err;
	}

	iter = pci_slot_match_iterator_create(slot);
	if (iter == NULL) {
		fprintf(stderr, "Iter creation failed\n");
		ret = EINVAL;
		goto err;
	}

	pdev = pci_device_next(iter);
	pci_iterator_destroy(iter);

	if (NULL == pdev) {
		fprintf(stderr, "No PCI device in specified slot %s\n",
			arg_str);
		ret = ENODEV;
		goto err;
	}

	ret = pci_device_probe(pdev);
	if (ret) {
		fprintf(stderr, "PCI dev %s probe error: %s\n", arg_str,
			strerror(ret));
		goto err;
	}

	if (!is_supported_chipset(aem, pdev)) {
		ret = ENOTSUP;
		goto err;
	}

	ret = pci_device_init(aem, pdev);
	if (ret)
		goto err;

	return 0;

err:
	pci_system_cleanup();

	return -ret;
}

static void pci_clean(struct atheepmgr *aem)
{
	pci_device_cleanup(aem);
	pci_system_cleanup();
}

const struct connector con_pci = {
	.name = "PCI",
	.priv_data_sz = sizeof(struct pci_priv),
	.caps = CON_CAP_HW,
	.init = pci_init,
	.clean = pci_clean,
	.reg_read = pci_reg_read,
	.reg_write = pci_reg_write,
	.reg_rmw = pci_reg_rmw,
};
