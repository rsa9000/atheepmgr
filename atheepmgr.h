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

#ifndef ATHEEPMGR_H
#define ATHEEPMGR_H

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

#define AR_SREV_VERSION_5418		0xC
#define AR_SREV_REVISION_5418		0
#define AR_SREV_VERSION_5416		0xD
#define AR_SREV_REVISION_5416		0
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

#define AR_SREV_5416_OR_LATER(_aem) \
	((_aem)->macVersion >= AR_SREV_VERSION_5418)
#define AR_SREV_9280_20_OR_LATER(_aem) \
	((_aem)->macVersion >= AR_SREV_VERSION_9280)
#define AR_SREV_9285(_aem) \
	((_aem)->macVersion == AR_SREV_VERSION_9285)
#define AR_SREV_9285_12_OR_LATER(_aem) \
	((_aem)->macVersion >= AR_SREV_VERSION_9285)
#define AR_SREV_9287(_aem) \
	((_aem)->macVersion == AR_SREV_VERSION_9287)
#define AR_SREV_9287_11_OR_LATER(_aem) \
	((_aem)->macVersion >= AR_SREV_VERSION_9287)
#define AR_SREV_9300_20_OR_LATER(_aem) \
	((_aem)->macVersion >= AR_SREV_VERSION_9300)
#define AR_SREV_9485(_aem) \
	((_aem)->macVersion == AR_SREV_VERSION_9485)
#define AR_SREV_9330(_aem) \
	((_aem)->macVersion == AR_SREV_VERSION_9330)
#define AR_SREV_9340(_aem) \
	((_aem)->macVersion == AR_SREV_VERSION_9340)
#define AR_SREV_9462(_aem) \
	((_aem)->macVersion == AR_SREV_VERSION_9462)
#define AR_SREV_9550(_aem) \
	((_aem)->macVersion == AR_SREV_VERSION_9550)
#define AR_SREV_9565(_aem) \
	((_aem)->macVersion == AR_SREV_VERSION_9565)

#define AR9XXX_GPIO_IN_OUT	(AR_SREV_9340(aem) ? 0x4028 : 0x4048)

#define AR5416_GPIO_IN_VAL	0x0FFFC000
#define AR5416_GPIO_IN_VAL_S	14
#define AR9280_GPIO_IN_VAL	0x000FFC00
#define AR9280_GPIO_IN_VAL_S	10
#define AR9285_GPIO_IN_VAL	0x00FFF000
#define AR9285_GPIO_IN_VAL_S	12
#define AR9287_GPIO_IN_VAL	0x003FF800
#define AR9287_GPIO_IN_VAL_S	11
#define AR9300_GPIO_IN_VAL	0x0001FFFF
#define AR9300_GPIO_IN_VAL_S	0

#define AR9XXX_GPIO_OE_OUT	(AR_SREV_9340(aem) ? 0x4030 : \
				 (AR_SREV_9300_20_OR_LATER(aem) ? 0x4050 : \
				  0x404c))

#define AR9XXX_GPIO_OE_OUT_DRV		0x3
#define AR9XXX_GPIO_OE_OUT_DRV_NO	0x0
#define AR9XXX_GPIO_OE_OUT_DRV_LOW	0x1
#define AR9XXX_GPIO_OE_OUT_DRV_HI	0x2
#define AR9XXX_GPIO_OE_OUT_DRV_ALL	0x3

#define AR9XXX_GPIO_OUTPUT_MUX1	(AR_SREV_9340(aem) ? 0x4048 : \
				 (AR_SREV_9300_20_OR_LATER(aem) ? 0x4068 : \
				  0x4060))
#define AR9XXX_GPIO_OUTPUT_MUX2	(AR_SREV_9340(aem) ? 0x404C : \
				 (AR_SREV_9300_20_OR_LATER(aem) ? 0x406C : \
				  0x4064))
#define AR9XXX_GPIO_OUTPUT_MUX3	(AR_SREV_9340(aem) ? 0x4050 : \
				 (AR_SREV_9300_20_OR_LATER(aem) ? 0x4070 : \
				  0x4068))

#define AR9XXX_GPIO_OUTPUT_MUX_MASK		0x1f
#define AR9XXX_GPIO_OUTPUT_MUX_OUTPUT		0x00
#define AR9XXX_GPIO_OUTPUT_MUX_TX_FRAME		0x03
#define AR9XXX_GPIO_OUTPUT_MUX_RX_CLEAR		0x04
#define AR9XXX_GPIO_OUTPUT_MUX_MAC_NETWORK	0x05
#define AR9XXX_GPIO_OUTPUT_MUX_MAC_POWER	0x06

#define AH_WAIT_TIMEOUT 100000 /* (us) */
#define AH_TIME_QUANTUM 10

#define CON_CAP_HW		1	/* Con. is able to interact with HW */

#define EEP_WP_GPIO_AUTO	-1	/* Use autodetection */
#define EEP_WP_GPIO_NONE	-2	/* Do not use GPIO for unlocking */

struct atheepmgr;

struct gpio_ops {
	int (*input_get)(struct atheepmgr *aem, unsigned gpio);
	int (*output_get)(struct atheepmgr *aem, unsigned gpio);
	void (*output_set)(struct atheepmgr *aem, unsigned gpio, int val);
	void (*dir_set_out)(struct atheepmgr *aem, unsigned gpio);
	const char * (*dir_get_str)(struct atheepmgr *aem, unsigned gpio);
	const char * (*out_mux_get_str)(struct atheepmgr *aem, unsigned gpio);
};

struct connector {
	const char *name;
	size_t priv_data_sz;
	unsigned int caps;
	int (*init)(struct atheepmgr *aem, const char *arg_str);
	void (*clean)(struct atheepmgr *aem);
	uint32_t (*reg_read)(struct atheepmgr *aem, uint32_t reg);
	void (*reg_write)(struct atheepmgr *aem, uint32_t reg, uint32_t val);
	void (*reg_rmw)(struct atheepmgr *aem, uint32_t reg, uint32_t set,
			uint32_t clr);
	bool (*eep_read)(struct atheepmgr *aem, uint32_t off, uint16_t *data);
	bool (*eep_write)(struct atheepmgr *aem, uint32_t off, uint16_t data);
	void (*eep_lock)(struct atheepmgr *aem, int lock);
};

enum eepmap_section_id {
	EEP_SECT_INIT,
	EEP_SECT_BASE,
	EEP_SECT_MODAL,
	EEP_SECT_POWER,
	__EEP_SECT_MAX
};
#define EEP_SECT_MAX			(__EEP_SECT_MAX)

enum eepmap_param_id {
	EEP_UPDATE_MAC,			/* Update device MAC address */
	__EEP_PARAM_MAX
};

struct eepmap {
	const char *name;
	const char *desc;
	size_t priv_data_sz;
	size_t eep_buf_sz;		/* EEP buffer size in 16-bit words */
	bool (*fill_eeprom)(struct atheepmgr *aem);
	int (*check_eeprom)(struct atheepmgr *aem);
	void (*dump[EEP_SECT_MAX])(struct atheepmgr *aem);
	bool (*update_eeprom)(struct atheepmgr *aem, int param,
			      const void *data);
	int params_mask;		/* Mask of updateable params */
};

struct atheepmgr {
	int host_is_be;				/* Is host big-endian? */

	const struct connector *con;
	void *con_priv;

	uint32_t macVersion;
	uint16_t macRev;

	const struct eepmap *eepmap;
	void *eepmap_priv;

	int eep_io_swap;			/* Swap words */
	uint16_t *eep_buf;			/* Intermediated EEPROM buf */

	int eep_wp_gpio_num;			/* EEPROM WP GPIO number */
	int eep_wp_gpio_pol;			/* EEPROM WP unlock polarity */

	const struct gpio_ops *gpio;
	unsigned gpio_num;			/* Number of GPIO lines */
};

extern const struct connector con_file;
extern const struct connector con_mem;
extern const struct connector con_pci;

extern const struct eepmap eepmap_5416;
extern const struct eepmap eepmap_9285;
extern const struct eepmap eepmap_9287;
extern const struct eepmap eepmap_9300;

bool hw_wait(struct atheepmgr *aem, uint32_t reg, uint32_t mask,
	     uint32_t val, uint32_t timeout);
bool hw_eeprom_read_9xxx(struct atheepmgr *aem, uint32_t off, uint16_t *data);
bool hw_eeprom_write_9xxx(struct atheepmgr *aem, uint32_t off, uint16_t data);
void hw_eeprom_lock_gpio(struct atheepmgr *aem, bool lock);
bool hw_eeprom_read(struct atheepmgr *aem, uint32_t off, uint16_t *data);
bool hw_eeprom_write(struct atheepmgr *aem, uint32_t off, uint16_t data);
void hw_eeprom_lock(struct atheepmgr *aem, int lock);
int hw_init(struct atheepmgr *aem);

#define EEP_READ(_off, _data)		\
		hw_eeprom_read(aem, _off, _data)
#define EEP_WRITE(_off, _data)		\
		hw_eeprom_write(aem, _off, _data)
#define EEP_LOCK()			\
		hw_eeprom_lock(aem, 1)
#define EEP_UNLOCK()			\
		hw_eeprom_lock(aem, 0)
#define REG_READ(_reg)			\
		aem->con->reg_read(aem, _reg)
#define REG_WRITE(_reg, _val)		\
		aem->con->reg_write(aem, _reg, _val)
#define REG_RMW(_reg, _set, _clr)	\
		aem->con->reg_rmw(aem, _reg, _set, _clr)

#endif /* ATHEEPMGR_H */
