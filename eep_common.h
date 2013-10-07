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

#ifndef EEP_COMMON_H
#define EEP_COMMON_H

#if __BYTE_ORDER == __BIG_ENDIAN
#define AR5416_EEPROM_MAGIC 0x5aa5
#else
#define AR5416_EEPROM_MAGIC 0xa55a
#endif

#define AR5416_EEPROM_MAGIC_OFFSET      0x0
#define AR5416_EEP_NO_BACK_VER          0x1
#define AR5416_EEP_VER                  0xE

#define AR5416_EEPROM_S         2
#define AR5416_EEPROM_OFFSET    0x2000

#define AR_EEPROM_STATUS_DATA             (AR_SREV_9340(edump) ? 0x40c8 : \
					   (AR_SREV_9300_20_OR_LATER(edump) ? \
					    0x4084 : 0x407c))
#define AR_EEPROM_STATUS_DATA_VAL                0x0000ffff
#define AR_EEPROM_STATUS_DATA_VAL_S              0
#define AR_EEPROM_STATUS_DATA_BUSY               0x00010000
#define AR_EEPROM_STATUS_DATA_BUSY_ACCESS        0x00020000
#define AR_EEPROM_STATUS_DATA_PROT_ACCESS        0x00040000
#define AR_EEPROM_STATUS_DATA_ABSENT_ACCESS      0x00080000

#define AR5416_OPFLAGS_11A           0x01
#define AR5416_OPFLAGS_11G           0x02
#define AR5416_OPFLAGS_N_5G_HT40     0x04
#define AR5416_OPFLAGS_N_2G_HT40     0x08
#define AR5416_OPFLAGS_N_5G_HT20     0x10
#define AR5416_OPFLAGS_N_2G_HT20     0x20

#define AR5416_EEPMISC_BIG_ENDIAN    0x01
#define AR5416_EEP_MINOR_VER_3       0x3

#define AR_EEPROM_MODAL_SPURS   5
#define AR5416_NUM_PD_GAINS     4
#define AR5416_PD_GAIN_ICEPTS   5

#define FREQ2FBIN(f, is_2g)	((is_2g) ? (f) - 2300 : ((f) - 4800) / 5)
#define FBIN2FREQ(b, is_2g)	((is_2g) ? (b) + 2300 : (b) * 5 + 4800)

extern const char * const sDeviceType[];

struct ar5416_spur_chan {
	uint16_t spurChan;
	uint8_t spurRangeLow;
	uint8_t spurRangeHigh;
} __attribute__ ((packed));

struct ar5416_cal_ctl_edges {
	uint8_t bChannel;
	uint8_t ctl;
} __attribute__ ((packed));

struct ar5416_cal_target_power_leg {
	uint8_t bChannel;
	uint8_t tPow2x[4];
} __attribute__ ((packed));

struct ar5416_cal_target_power_ht {
	uint8_t bChannel;
	uint8_t tPow2x[8];
} __attribute__ ((packed));

#define EEP_PRINT_SECT_NAME(__name)			\
		printf("\n.----------------------.\n");	\
		printf("| %-20s |\n", __name);		\
		printf("'----------------------'\n\n");

#endif /* EEP_COMMON_H */
