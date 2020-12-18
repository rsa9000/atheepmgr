/*
 * Copyright (c) 2013,2020 Sergey Ryazanov <ryazanov.s.a@gmail.com>
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

#include "atheepmgr.h"

struct file_priv {
	FILE *fp;
	uint32_t data_len;	/* File data length */
	uint32_t ic_sz;		/* IC size for addr wrap emulation */
};

/* See: https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2 */
static uint32_t roundup_pow_of_2(uint32_t v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;

	return v + 1;
}

static uint32_t file_reg_read(struct atheepmgr *aem, uint32_t reg)
{
	fprintf(stderr, "confile: direct reg access is not supported\n");

	return 0;
}

static void file_reg_write(struct atheepmgr *aem, uint32_t reg, uint32_t val)
{
	fprintf(stderr, "confile: direct reg write is not supported\n");
}

static void file_reg_rmw(struct atheepmgr *aem, uint32_t reg, uint32_t set,
			 uint32_t clr)
{
	fprintf(stderr, "confile: direct reg RMW is not supported\n");
}

static int file_blob_getsize(struct atheepmgr *aem)
{
	struct file_priv *fpd = aem->con_priv;

	return fpd->data_len;
}

static int file_blob_read(struct atheepmgr *aem, void *buf, int len)
{
	struct file_priv *fpd = aem->con_priv;

	if (fseek(fpd->fp, 0, SEEK_SET) != 0)
		return -1;

	return fread(buf, 1, len, fpd->fp);
}

static bool file_eeprom_read(struct atheepmgr *aem, uint32_t off, uint16_t *data)
{
	struct file_priv *fpd = aem->con_priv;
	uint32_t pos = off * 2;

	pos = pos % fpd->ic_sz;		/* Emulate address wrap */

	if (pos >= fpd->data_len) {	/* Emulate empty area */
		*data = 0xffff;
		return true;
	}

	if (fseek(fpd->fp, pos, SEEK_SET) != 0)
		return false;

	if (fread(data, sizeof(uint16_t), 1, fpd->fp) != 1)
		return false;

	return true;
}

static bool file_eeprom_write(struct atheepmgr *aem, uint32_t off, uint16_t data)
{
	struct file_priv *fpd = aem->con_priv;
	static const uint16_t fill = 0xffff;
	uint32_t pos = off * 2, addr;

	pos = pos % fpd->ic_sz;		/* Emulate address wrap */

	if (pos >= fpd->data_len) {
		/* Fill the empty area before writing position */
		if (fseek(fpd->fp, fpd->data_len, SEEK_SET) != 0)
			return false;
		for (addr = fpd->data_len; addr < pos; addr += sizeof(uint16_t))
			if (fwrite(&fill, sizeof(uint16_t), 1, fpd->fp) != 1)
				return false;
		fpd->data_len = pos + sizeof(uint16_t);	/* NB: with new data */
	} else {
		/* Immediatly go to writing position */
		if (fseek(fpd->fp, pos, SEEK_SET) != 0)
			return false;
	}

	if (fwrite(&data, sizeof(uint16_t), 1, fpd->fp) != 1)
		return false;

	return true;
}

static bool file_otp_read(struct atheepmgr *aem, uint32_t off, uint8_t *data)
{
	struct file_priv *fpd = aem->con_priv;

	if (off >= fpd->data_len) {	/* Emulate empty area */
		*data = 0x00;
		return true;
	}

	if (fseek(fpd->fp, off, SEEK_SET) != 0)
		return false;

	if (fread(data, sizeof(*data), 1, fpd->fp) != 1)
		return false;

	return true;
}

static int file_init(struct atheepmgr *aem, const char *arg_str)
{
	struct file_priv *fpd = aem->con_priv;
	int err;
	long len;

	fpd->fp = fopen(arg_str, "r+b");
	if (!fpd->fp) {
		fprintf(stderr, "confile: can not open dump file '%s': %s\n",
			arg_str, strerror(errno));
		goto err;
	}

	if (fseek(fpd->fp, 0, SEEK_END)) {
		fprintf(stderr, "confile: can not seek to the file end: %s\n",
			strerror(errno));
		goto err;
	}

	len = ftell(fpd->fp);
	if (len < 0) {
		fprintf(stderr, "confile: can not detect file size: %s\n",
			strerror(errno));
		goto err;
	}

	fpd->data_len = len & ~1;	/* Align to 16 bit */
	fpd->ic_sz = roundup_pow_of_2(fpd->data_len);
	if (fpd->ic_sz < 0x0800)	/* Do not emulate too small IC */
		fpd->ic_sz = 0x0800;

	if (aem->verbose)
		printf("confile: file data length is 0x%04lx (%ld) bytes, emulate 0x%04x bytes (%u KB, %u kbit) EEPROM IC\n",
		       len, len, fpd->ic_sz, fpd->ic_sz / 1024,
		       fpd->ic_sz * 8 / 1024);

	return 0;

err:
	err = errno;
	if (fpd->fp)
		fclose(fpd->fp);

	return -err;
}

static void file_clean(struct atheepmgr *aem)
{
	struct file_priv *fpd = aem->con_priv;

	fclose(fpd->fp);
}

static const struct blob_ops blob_file = {
	.getsize = file_blob_getsize,
	.read = file_blob_read,
};

static const struct eep_ops eep_file = {
	.read = file_eeprom_read,
	.write = file_eeprom_write,
};

static const struct otp_ops otp_file = {
	.read = file_otp_read,
};

const struct connector con_file = {
	.name = "File",
	.priv_data_sz = sizeof(struct file_priv),
	.init = file_init,
	.clean = file_clean,
	.reg_read = file_reg_read,
	.reg_write = file_reg_write,
	.reg_rmw = file_reg_rmw,
	.blob = &blob_file,
	.eep = &eep_file,
	.otp = &otp_file,
};
