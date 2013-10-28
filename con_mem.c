/*
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "atheepmgr.h"

struct mem_priv {
	int devmem_fd;
	off_t io_addr;
	void *io_map;
};

static uint32_t mem_reg_read(struct atheepmgr *aem, uint32_t reg)
{
	struct mem_priv *mpd = aem->con_priv;

	return *((volatile uint32_t *)(mpd->io_map + reg));
}

static void mem_reg_write(struct atheepmgr *aem, uint32_t reg, uint32_t val)
{
	struct mem_priv *mpd = aem->con_priv;

	*((volatile uint32_t *)(mpd->io_map + reg)) = val;
}

static void mem_reg_rmw(struct atheepmgr *aem, uint32_t reg, uint32_t set,
			uint32_t clr)
{
	struct mem_priv *mpd = aem->con_priv;
	uint32_t tmp;

	tmp = *((volatile uint32_t *)(mpd->io_map + reg));
	tmp &= ~clr;
	tmp |= set;
	*((volatile uint32_t *)(mpd->io_map + reg)) = tmp;
}

static int mem_init(struct atheepmgr *aem, const char *arg_str)
{
	struct mem_priv *mpd = aem->con_priv;
	static const size_t mem_size = 0x10000;	/* TODO: use autodetection */
	char *endp;

	errno = 0;
	mpd->io_addr = strtoul(arg_str, &endp, 16);
	if (!mpd->io_addr || *endp != '\0' || errno || mpd->io_addr % 4 != 0) {
		fprintf(stderr, "conmem: invalid I/O memory start address -- %s\n",
			arg_str);
		return -EINVAL;
	}

	mpd->devmem_fd = open("/dev/mem", O_RDWR);
	if (mpd->devmem_fd < 0) {
		fprintf(stderr, "conmem: opening /dev/mem failed: %s\n",
			strerror(errno));
		return -errno;
	}

	mpd->io_map = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
			   MAP_SHARED | MAP_FILE, mpd->devmem_fd, mpd->io_addr);
	if (MAP_FAILED == mpd->io_map) {
		fprintf(stderr, "conmem: mmap of device at 0x%08lx for 0x%08lx bytes failed: %s\n",
			(unsigned long)mpd->io_addr,
			(unsigned long)mem_size, strerror(errno));
		close(mpd->devmem_fd);
		return -errno;
	}

	return 0;
}

static void mem_clean(struct atheepmgr *aem)
{
	struct mem_priv *mpd = aem->con_priv;

	close(mpd->devmem_fd);
}

const struct connector con_mem = {
	.name = "Mem",
	.priv_data_sz = sizeof(struct mem_priv),
	.caps = CON_CAP_HW,
	.init = mem_init,
	.clean = mem_clean,
	.reg_read = mem_reg_read,
	.reg_write = mem_reg_write,
	.reg_rmw = mem_reg_rmw,
	.eep_read = hw_eeprom_read_9xxx,
	.eep_write = hw_eeprom_write_9xxx,
	.eep_lock = hw_eeprom_lock_gpio,
};

