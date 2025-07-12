/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * Copyright (c) 2013,2017-2020 Sergey Ryazanov <ryazanov.s.a@gmail.com>
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

#if defined(__OpenBSD__)
#include <sys/param.h>
/* OpenBSD only starting from version 5.6 contains le16toh() and le32toh() */
#if OpenBSD >= 201411
#include <endian.h>
#else
#define le16toh		letoh16
#define le32toh		letoh32
#endif
#define __BYTE_ORDER _BYTE_ORDER
#define __BIG_ENDIAN _BIG_ENDIAN
#define bswap_16	__swap16
#define bswap_32	__swap32
#elif defined(__FreeBSD__)
#include <sys/endian.h>
#define __BYTE_ORDER _BYTE_ORDER
#define __BIG_ENDIAN _BIG_ENDIAN
#define bswap_16	bswap16
#define bswap_32	bswap32
#elif defined(__linux__)
#include <endian.h>
#include <byteswap.h>
/* Some old C libraries (e.g. uClibc prior to 0.9.32) do not have this */
#ifndef le16toh
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define le16toh(x)	(x)
#define htole16(x)	(x)
#define le32toh(x)	(x)
#define htole32(x)	(x)
#else
#define le16toh(x)	bswap_16(x)
#define htole16(x)	bswap_16(x)
#define le32toh(x)	bswap_32(x)
#define htole32(x)	bswap_32(x)
#endif
#endif
#endif

#define bswap_16_inplace(__x)	(__x) = bswap_16(__x)
#define bswap_32_inplace(__x)	(__x) = bswap_32(__x)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define MS(_v, _f)  (((_v) & _f) >> _f##_S)
#define BIT(_n)				(1 << (_n))
#define offsetof(_type, _member)	__builtin_offsetof(_type, _member)

enum {
	false = 0,
	true = 1
};

typedef int bool;

#define AR_SREV_VERSION_5211		0x4
#define AR_SREV_REVISION_5211		0
#define AR_SREV_VERSION_5212		0x5
#define AR_SREV_REVISION_5212		0
#define AR_SREV_VERSION_5213		0x5
#define AR_SREV_REVISION_5213		5
#define AR_SREV_VERSION_5213A		0x5
#define AR_SREV_REVISION_5213A		9
#define AR_SREV_VERSION_5413		0xA
#define AR_SREV_REVISION_5413		4
#define AR_SREV_VERSION_5414		0xA
#define AR_SREV_REVISION_5414		5
#define AR_SREV_VERSION_5418		0xC
#define AR_SREV_REVISION_5418		0
#define AR_SREV_VERSION_5416		0xD
#define AR_SREV_REVISION_5416		0
#define AR_SREV_VERSION_9160		0x40
#define AR_SREV_VERSION_9280		0x80
#define AR_SREV_VERSION_9285		0xC0
#define AR_SREV_VERSION_9287		0x180
#define AR_SREV_VERSION_9300		0x1c0
#define AR_SREV_VERSION_9330		0x200
#define AR_SREV_VERSION_9485		0x240
#define AR_SREV_VERSION_9462		0x280
#define AR_SREV_VERSION_9565		0x2c0
#define AR_SREV_VERSION_9340		0x300
#define AR_SREV_VERSION_9550		0x400
#define AR_SREV_VERSION_9880		0x4300

#define AR_SREV_5211_OR_LATER(_aem) \
	((_aem)->macVersion >= AR_SREV_VERSION_5211)
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
#define AR_SREV_AFTER_9550(_aem) \
	((_aem)->macVersion > AR_SREV_VERSION_9550)
#define AR_SREV_9880(_aem) \
	((_aem)->macVersion == AR_SREV_VERSION_9880)

#define AH_WAIT_TIMEOUT		100000 /* (us) */
#define AH_TIME_QUANTUM		10

#define CON_CAP_HW		1	/* Con. is able to interact with HW */
#define CON_CAP_PNP		2	/* Con. is able to detect EEP layout */

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

struct blob_ops {
	int (*getsize)(struct atheepmgr *aem);
	int (*read)(struct atheepmgr *aem, void *buf, int len);
};

struct eep_ops {
	bool (*read)(struct atheepmgr *aem, uint32_t off, uint16_t *data);
	bool (*write)(struct atheepmgr *aem, uint32_t off, uint16_t data);
	void (*lock)(struct atheepmgr *aem, int lock);
};

struct otp_ops {
	bool (*enable)(struct atheepmgr *aem, int enable);
	bool (*read)(struct atheepmgr *aem, uint32_t off, uint8_t *data);
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
	const struct blob_ops *blob;
	const struct eep_ops *eep;
	const struct otp_ops *otp;
};

enum eepmap_feature {
	EEPMAP_F_RAW_EEP = 1 << 0,	/* Support RAW EEPROM loading */
	EEPMAP_F_RAW_OTP = 1 << 1,	/* Support RAW OTP mem loading */
};
#define EEPMAP_F_RAW_DATA	(EEPMAP_F_RAW_EEP | EEPMAP_F_RAW_OTP)

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
	EEP_ERASE_CTL,			/* Erase CTL (Conformance Test Limit) */
	__EEP_PARAM_MAX
};

struct eeptemplate {
	int id;			/* Templ. id as specified in the comp. header */
	const char *name;	/* Template name for user */
	const void *data;	/* Template data pointer */
};

struct eepmap {
	const char *name;
	const char *desc;
	int features;			/* Supported features */
	struct chip_regs {
		uint32_t srev;
	} chip_regs;
	size_t priv_data_sz;
	size_t eep_buf_sz;		/* EEP buffer size in 16-bit words */
	size_t unpacked_buf_sz;		/* Buffer size for unpacked data */
	const struct eeptemplate *templates;	/* NULL terminated list */
	bool (*load_blob)(struct atheepmgr *aem);
	bool (*load_eeprom)(struct atheepmgr *aem, bool raw);
	bool (*load_otp)(struct atheepmgr *aem, bool raw);
	bool (*check_eeprom)(struct atheepmgr *aem);
	void (*dump[EEP_SECT_MAX])(struct atheepmgr *aem);
	bool (*update_eeprom)(struct atheepmgr *aem, int param,
			      const void *data);
	int params_mask;		/* Mask of updateable params */
};

struct chip_pciid {
	uint16_t dev_id;
};

struct chip {
	const char *name;
	const struct eepmap *eepmap;
	struct chip_pciid pciids[4];	/* Allow multiple IDs */
};

struct atheepmgr {
	int verbose;

	int host_is_be;				/* Is host big-endian? */

	const struct connector *con;
	void *con_priv;

	uint32_t macVersion;
	uint16_t macRev;

	const struct eepmap *eepmap;
	void *eepmap_priv;

	int eep_io_swap;			/* Swap words */
	uint16_t *eep_buf;			/* Intermediated EEPROM buf */
	size_t eep_len;			/* Read size of EEPROM data in the buffer */

	uint8_t *unpacked_buf;			/* Buffer for unpacked data */
	size_t unpacked_len;			/* Unpacked data length */

	int eep_wp_gpio_num;			/* EEPROM WP GPIO number */
	int eep_wp_gpio_pol;			/* EEPROM WP unlock polarity */

	const struct eep_ops *eep;

	const struct otp_ops *otp;
	int otp_was_enabled;

	const struct gpio_ops *gpio;
	unsigned gpio_num;			/* Number of GPIO lines */
};

extern const struct connector con_file;
extern const struct connector con_driver;
extern const struct connector con_mem;
extern const struct connector con_pci;
extern const struct connector con_stub;

extern const struct eepmap eepmap_5211;
extern const struct eepmap eepmap_5416;
extern const struct eepmap eepmap_6174;
extern const struct eepmap eepmap_9285;
extern const struct eepmap eepmap_9287;
extern const struct eepmap eepmap_9300;
extern const struct eepmap eepmap_9880;
extern const struct eepmap eepmap_9888;

int chips_find_by_pci_id(uint16_t dev_id, const struct chip *res[], int nmemb);

bool hw_wait(struct atheepmgr *aem, uint32_t reg, uint32_t mask,
	     uint32_t val, uint32_t timeout);
void hw_eeprom_set_ops(struct atheepmgr *aem);
bool hw_eeprom_read(struct atheepmgr *aem, uint32_t off, uint16_t *data);
bool hw_eeprom_write(struct atheepmgr *aem, uint32_t off, uint16_t data);
void hw_eeprom_lock(struct atheepmgr *aem, int lock);
void hw_otp_set_ops(struct atheepmgr *aem);
bool hw_otp_enable(struct atheepmgr *aem, int enable);
bool hw_otp_read(struct atheepmgr *aem, uint32_t off, uint8_t *data);
int hw_init(struct atheepmgr *aem);

#define EEP_READ(_off, _data)		\
		hw_eeprom_read(aem, _off, _data)
#define EEP_WRITE(_off, _data)		\
		hw_eeprom_write(aem, _off, _data)
#define EEP_LOCK()			\
		hw_eeprom_lock(aem, 1)
#define EEP_UNLOCK()			\
		hw_eeprom_lock(aem, 0)
#define OTP_ENABLE()			\
		hw_otp_enable(aem, 1)
#define OTP_DISABLE()			\
		hw_otp_enable(aem, 0);
#define OTP_READ(_off, _data)		\
		hw_otp_read(aem, _off, _data)
#define REG_READ(_reg)			\
		aem->con->reg_read(aem, _reg)
#define REG_WRITE(_reg, _val)		\
		aem->con->reg_write(aem, _reg, _val)
#define REG_RMW(_reg, _set, _clr)	\
		aem->con->reg_rmw(aem, _reg, _set, _clr)

#endif /* ATHEEPMGR_H */
