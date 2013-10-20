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

#ifndef EDUMP_H
#define EDUMP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <endian.h>
#include <byteswap.h>

#include "eep_common.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define MS(_v, _f)  (((_v) & _f) >> _f##_S)
#define BIT(_n)				(1 << (_n))
#define offsetof(_type, _member)	__builtin_offsetof(_type, _member)

enum {
	false = 0,
	true = 1
};

typedef int bool;

#define AR_SREV                 0x4020
#define AR_SREV_ID              0x000000FF
#define AR_SREV_VERSION         0x000000F0
#define AR_SREV_VERSION_S       4
#define AR_SREV_REVISION        0x00000007
#define AR_SREV_VERSION2        0xFFFC0000
#define AR_SREV_VERSION2_S      18
#define AR_SREV_TYPE2           0x0003F000
#define AR_SREV_TYPE2_S         12
#define AR_SREV_REVISION2       0x00000F00
#define AR_SREV_REVISION2_S     8

#define AR_SREV_VERSION_5416_PCI        0xD
#define AR_SREV_VERSION_5416_PCIE       0xC
#define AR_SREV_VERSION_9160            0x40
#define AR_SREV_VERSION_9280            0x80
#define AR_SREV_VERSION_9285            0xC0
#define AR_SREV_VERSION_9287            0x180
#define AR_SREV_VERSION_9300            0x1c0
#define AR_SREV_VERSION_9330            0x200
#define AR_SREV_VERSION_9485            0x240
#define AR_SREV_VERSION_9462            0x280
#define AR_SREV_VERSION_9565            0x2c0
#define AR_SREV_VERSION_9340            0x300
#define AR_SREV_VERSION_9550            0x400

#define AR_SREV_9280_20_OR_LATER(edump) \
	(((edump)->macVersion >= AR_SREV_VERSION_9280))
#define AR_SREV_9285(_ah) \
	(((edump)->macVersion == AR_SREV_VERSION_9285))
#define AR_SREV_9287(_ah) \
	(((edump)->macVersion == AR_SREV_VERSION_9287))
#define AR_SREV_9300_20_OR_LATER(edump) \
	(((edump)->macVersion >= AR_SREV_VERSION_9300))
#define AR_SREV_9485(edump) \
	(((edump)->macVersion == AR_SREV_VERSION_9485))
#define AR_SREV_9330(edump) \
	(((edump)->macVersion == AR_SREV_VERSION_9330))
#define AR_SREV_9340(edump) \
	(((edump)->macVersion == AR_SREV_VERSION_9340))
#define AR_SREV_9462(edump) \
	(((edump)->macVersion == AR_SREV_VERSION_9462))
#define AR_SREV_9550(edump) \
	(((edump)->macVersion == AR_SREV_VERSION_9550))
#define AR_SREV_9565(edump) \
	(((edump)->macVersion == AR_SREV_VERSION_9565))

#define AH_WAIT_TIMEOUT 100000 /* (us) */
#define AH_TIME_QUANTUM 10

#define CON_CAP_HW		1	/* Con. is able to interact with HW */

struct edump;

struct connector {
	const char *name;
	size_t priv_data_sz;
	unsigned int caps;
	int (*init)(struct edump *edump, const char *arg_str);
	void (*clean)(struct edump *edump);
	uint32_t (*reg_read)(struct edump *edump, uint32_t reg);
	void (*reg_write)(struct edump *edump, uint32_t reg, uint32_t val);
	bool (*eep_read)(struct edump *edump, uint32_t off, uint16_t *data);
	bool (*eep_write)(struct edump *edump, uint32_t off, uint16_t data);
};

enum eepmap_param_id {
	EEP_UPDATE_MAC,			/* Update device MAC address */
	__EEP_PARAM_MAX
};

struct eepmap {
	const char *name;
	const char *desc;
	size_t priv_data_sz;
	size_t eep_buf_sz;		/* EEP buffer size in 16-bit words */
	bool (*fill_eeprom)(struct edump *edump);
	int (*check_eeprom)(struct edump *edump);
	void (*dump_base_header)(struct edump *edump);
	void (*dump_modal_header)(struct edump *edump);
	void (*dump_power_info)(struct edump *edump);
	bool (*update_eeprom)(struct edump *edump, int param,
			      const void *data);
	int params_mask;		/* Mask of updateable params */
};

struct edump {
	int host_is_be;				/* Is host big-endian? */

	const struct connector *con;
	void *con_priv;

	uint32_t macVersion;
	uint16_t macRev;

	const struct eepmap *eepmap;
	void *eepmap_priv;

	int eep_io_swap;			/* Swap words */
	uint16_t *eep_buf;			/* Intermediated EEPROM buf */
};

extern const struct connector con_file;
extern const struct connector con_mem;
extern const struct connector con_pci;

extern const struct eepmap eepmap_5416;
extern const struct eepmap eepmap_9285;
extern const struct eepmap eepmap_9287;
extern const struct eepmap eepmap_9300;

void hw_read_revisions(struct edump *edump);
bool hw_wait(struct edump *edump, uint32_t reg, uint32_t mask,
	     uint32_t val, uint32_t timeout);
bool hw_eeprom_read_9xxx(struct edump *edump, uint32_t off, uint16_t *data);
bool hw_eeprom_write_9xxx(struct edump *edump, uint32_t off, uint16_t data);
bool hw_eeprom_read(struct edump *edump, uint32_t off, uint16_t *data);
bool hw_eeprom_write(struct edump *edump, uint32_t off, uint16_t data);

#define EEP_READ(_off, _data)		\
		hw_eeprom_read(edump, _off, _data)
#define EEP_WRITE(_off, _data)		\
		hw_eeprom_write(edump, _off, _data)
#define REG_READ(_reg)			\
		edump->con->reg_read(edump, _reg)
#define REG_WRITE(_reg, _val)		\
		edump->con->reg_write(edump, _reg, _val)

#endif /* EDUMP_H */
