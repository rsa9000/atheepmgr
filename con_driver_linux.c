/*
 * Copyright (c) 2020 Sergey Ryazanov <ryazanov.s.a@gmail.com>
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

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "atheepmgr.h"

#define SYSFS_NETDEV_PATH "/sys/class/net"
#define SYSFS_CFG80211_PATH "/sys/class/ieee80211"
#define DEBUGFS_PATH "/sys/kernel/debug"
#define DEBUGFS_CFG80211_PATH DEBUGFS_PATH "/ieee80211"

struct driver_priv {
	char *regidx_fname;
	char *regval_fname;
	const char *regval_fmt;
	int regval_strlen;
};

static const char * const driver_ath9k_names[] = {
	"ath9k",
	NULL
};

static const char * const driver_ath10k_names[] = {
	"ath10k_ahb",
	"ath10k_pci",
	NULL
};

static const struct driver_info {
	const char * const name;
	const char * const *driver_names;
	struct {
		const char * const regidx_fname;
		const char * const regval_fname;
		const char * const regval_fmt;
		int regval_strlen;
	} debugfs;
} driver_infos[] = {
	{
		.name = "ath9k",
		.driver_names = driver_ath9k_names,
		.debugfs = {
			.regidx_fname = "regidx",
			.regval_fname = "regval",
			.regval_fmt = "0x%8x%n",
			.regval_strlen = 10,
		},
	}, {
		.name = "ath10k",
		.driver_names = driver_ath10k_names,
		.debugfs = {
			.regidx_fname = "reg_addr",
			.regval_fname = "reg_value",
			.regval_fmt = "0x%*8x:0x%8x%n",
			.regval_strlen = 21,
		},
	}
};

static int __regidx_write(struct atheepmgr *aem, uint32_t reg)
{
	struct driver_priv *dpd = aem->con_priv;
	FILE *fp = fopen(dpd->regidx_fname, "w");

	if (!fp) {
		fprintf(stderr, "condriver: unable to open %s for writing: %s\n",
			dpd->regidx_fname, strerror(errno));
		return -1;
	}
	if (fprintf(fp, "0x%08x\n", reg) != 11) {
		fprintf(stderr, "condriver: unable to write register address: %s\n",
			strerror(errno));
		fclose(fp);
		return -1;
	}
	fclose(fp);

	return 0;
}

static int __regval_read(struct atheepmgr *aem, uint32_t *pval)
{
	struct driver_priv *dpd = aem->con_priv;
	FILE *fp = fopen(dpd->regval_fname, "r");
	unsigned int v;
	int n, l;

	if (!fp) {
		fprintf(stderr, "condriver: unable to open %s for reading: %s\n",
			dpd->regval_fname, strerror(errno));
		return -1;
	}
	n = fscanf(fp, dpd->regval_fmt, &v, &l);
	if (ferror(fp)) {
		fprintf(stderr, "condriver: unable to read register value file: %s\n",
			strerror(errno));
		fclose(fp);
		return -1;
	}
	fclose(fp);
	if (n != 1 || l != dpd->regval_strlen) {
		fprintf(stderr, "condriver: unexpected register value format\n");
		return -1;
	}
	*pval = v;

	return 0;
}

static int __regval_write(struct atheepmgr *aem, uint32_t val)
{
	struct driver_priv *dpd = aem->con_priv;
	FILE *fp = fopen(dpd->regval_fname, "w");

	if (!fp) {
		fprintf(stderr, "condriver: unable to open %s for writing: %s\n",
			dpd->regval_fname, strerror(errno));
		return -1;
	}
	if (fprintf(fp, "0x%08x\n", val) != 11) {
		fprintf(stderr, "condriver: unable to write register value: %s\n",
			strerror(errno));
		fclose(fp);
		return -1;
	}
	fclose(fp);

	return 0;
}

static uint32_t driver_reg_read(struct atheepmgr *aem, uint32_t reg)
{
	uint32_t value = 0;

	if (__regidx_write(aem, reg))
		goto err;
	if (__regval_read(aem, &value))
		goto err;

	return value;

err:
	return 0;
}

static void driver_reg_write(struct atheepmgr *aem, uint32_t reg, uint32_t val)
{
	if (__regidx_write(aem, reg))
		return;
	__regval_write(aem, val);
}

static void driver_reg_rmw(struct atheepmgr *aem, uint32_t reg, uint32_t set,
			   uint32_t clr)
{
	uint32_t value = 0;

	if (__regidx_write(aem, reg))
		return;
	if (__regval_read(aem, &value))
		return;

	value &= ~clr;
	value |= set;

	__regval_write(aem, value);
}

#define STATERRMSG(__path)						\
	fprintf(stderr, "condriver: unable to stat %s: %s\n", __path,	\
		strerror(errno))

#define TEST_DIR(__dirname, __noentmsg)					\
	do {								\
		if (stat(__dirname, &statbuf)) {			\
			STATERRMSG(__dirname);				\
			if (__noentmsg && errno == ENOENT)		\
				fprintf(stderr, "condriver: %s\n", __noentmsg);\
			goto err_dir;					\
		}							\
	} while (0);

static int driver_init(struct atheepmgr *aem, const char *arg_str)
{
	char *p, pbuf[0x100], phyname[0x20], drivername[0x40];
	struct driver_priv *dpd = aem->con_priv;
	const struct driver_info *di;
	struct stat statbuf;
	int i, j, res;

	TEST_DIR(DEBUGFS_PATH, "has the DebugFS been mounted?");
	TEST_DIR(SYSFS_CFG80211_PATH, "has cfg80211 module been loaded?");
	TEST_DIR(DEBUGFS_CFG80211_PATH,
		 "has cfg80211 been built with the debugfs support?");

	snprintf(pbuf, sizeof(pbuf), SYSFS_NETDEV_PATH "/%s", arg_str);
	if (stat(pbuf, &statbuf)) {
		if (errno == ENOENT) {
			strncpy(phyname, arg_str, sizeof(phyname));
			phyname[sizeof(phyname) - 1] = '\0';
			goto skip_netdev;
		}
		STATERRMSG(pbuf);
		return -1;
	}

	snprintf(pbuf, sizeof(pbuf), SYSFS_NETDEV_PATH "/%s/phy80211", arg_str);
	res = readlink(pbuf, phyname, sizeof(phyname));
	if (res < 0) {
		fprintf(stderr, "condriver: unable to read phy path from %s: %s\n",
			pbuf, strerror(errno));
		return -1;
	} else if (res >= sizeof(phyname)) {
		fprintf(stderr, "condriver: %s network device has too long IEEE 802.11 phy name\n",
			arg_str);
		return -1;
	}
	phyname[res] = '\0';
	p = strrchr(phyname, '/');
	if (!p) {
		fprintf(stderr, "condirver: unable to extract IEEE 802.11 phy name from %s path\n",
			phyname);
		return -1;
	}
	memmove(phyname, p + 1, res - (p - phyname));

skip_netdev:
	snprintf(pbuf, sizeof(pbuf), SYSFS_CFG80211_PATH"/%s", phyname);
	if (stat(pbuf, &statbuf)) {
		if (errno == ENOENT)
			fprintf(stderr, "condriver: no such IEEE 802.11 phy -- %s\n",
				phyname);
		else
			STATERRMSG(pbuf);
		return -1;
	}

	snprintf(pbuf, sizeof(pbuf), SYSFS_CFG80211_PATH"/%s/device/driver", phyname);
	res = readlink(pbuf, drivername, sizeof(drivername));
	if (res < 0) {
		fprintf(stderr, "condriver: unable to read phy driver name from %s: %s\n",
			pbuf, strerror(errno));
		return -1;
	} else if (res >= sizeof(drivername)) {
		fprintf(stderr, "condriver: %s phy has too long driver path\n",
			phyname);
		return -1;
	}

	drivername[res] = '\0';
	p = strrchr(drivername, '/');
	if (!p) {
		fprintf(stderr, "condirver: unable to extract phy driver name from %s path\n",
			drivername);
		return -1;
	}
	memmove(drivername, p + 1, res - (p - drivername));

	for (i = 0; i < ARRAY_SIZE(driver_infos); ++i) {
		di = &driver_infos[i];
		for (j = 0; di->driver_names[j] != NULL; ++j)
			if (strcmp(di->driver_names[j], drivername) == 0)
				break;
		if (di->driver_names[j] != NULL)
			break;
	}
	if (i == ARRAY_SIZE(driver_infos)) {
		fprintf(stderr, "condriver: phy is served by an unsupport driver -- %s\n",
			drivername);
		return -1;
	}

	snprintf(pbuf, sizeof(pbuf), DEBUGFS_CFG80211_PATH"/%s/%s/%s",
		 phyname, di->name, di->debugfs.regidx_fname);
	if (stat(pbuf, &statbuf)) {
		STATERRMSG(pbuf);
		if (errno == ENOENT)
			fprintf(stderr, "condriver: has driver %s been built without debugfs support?",
				di->name);
		goto err;
	}
	dpd->regidx_fname = strdup(pbuf);
	if (!dpd->regidx_fname) {
		fprintf(stderr, "condriver: unable to allocate memory for register address file path\n");
		goto err;
	}

	snprintf(pbuf, sizeof(pbuf), DEBUGFS_CFG80211_PATH"/%s/%s/%s",
		 phyname, di->name, di->debugfs.regval_fname);
	if (stat(pbuf, &statbuf)) {
		STATERRMSG(pbuf);
		if (errno == ENOENT)
			fprintf(stderr, "condriver: has driver %s been built without debugfs support?",
				di->name);
		goto err;
	}
	dpd->regval_fname = strdup(pbuf);
	if (!dpd->regval_fname) {
		fprintf(stderr, "condriver: unable to allocate memory for register value file path\n");
		goto err;
	}

	dpd->regval_fmt = di->debugfs.regval_fmt;
	dpd->regval_strlen = di->debugfs.regval_strlen;

	return 0;

err:
	free(dpd->regidx_fname);
	free(dpd->regval_fname);

err_dir:
	return -1;
}

static void driver_clean(struct atheepmgr *aem)
{
	struct driver_priv *dpd = aem->con_priv;

	free(dpd->regidx_fname);
	free(dpd->regval_fname);
}

const struct connector con_driver = {
	.name = "Driver",
	.priv_data_sz = sizeof(struct driver_priv),
	.caps = CON_CAP_HW,
	.init = driver_init,
	.clean = driver_clean,
	.reg_read = driver_reg_read,
	.reg_write = driver_reg_write,
	.reg_rmw = driver_reg_rmw,
};
