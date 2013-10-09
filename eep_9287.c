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
#include "eep_9287.h"

#define SIZE_EEPROM_AR9287 (sizeof(struct ar9287_eeprom) / sizeof(uint16_t))

struct eep_9287_priv {
	struct ar9287_eeprom eep;
};

static int eep_9287_get_ver(struct eep_9287_priv *emp)
{
	return (emp->eep.baseEepHeader.version >> 12) & 0xF;
}

static int eep_9287_get_rev(struct eep_9287_priv *emp)
{
	return (emp->eep.baseEepHeader.version) & 0xFFF;
}

static bool eep_9287_fill_eeprom(struct edump *edump)
{
	struct eep_9287_priv *emp = edump->eepmap_priv;
	uint16_t *eep_data = (uint16_t *)&emp->eep;
	int addr, eep_start_loc = 0;

	eep_start_loc = AR9287_EEP_START_LOC;

	for (addr = 0; addr < SIZE_EEPROM_AR9287; addr++) {
		if (!EEP_READ(addr + eep_start_loc, eep_data)) {
			fprintf(stderr, "Unable to read eeprom region\n");
			return false;
		}
		eep_data++;
	}

	return true;
}

static bool eep_9287_check_eeprom(struct edump *edump)
{
	struct eep_9287_priv *emp = edump->eepmap_priv;
	struct ar9287_eeprom *eep = &emp->eep;
	uint16_t *eepdata, temp, magic, magic2;
	uint32_t sum = 0, el;
	bool need_swap = false;
	int i, addr;

	if (!EEP_READ(AR5416_EEPROM_MAGIC_OFFSET, &magic)) {
		fprintf(stderr, "Reading Magic # failed\n");
		return false;
	}

	if (magic != AR5416_EEPROM_MAGIC) {
		magic2 = bswap_16(magic);

		if (magic2 == AR5416_EEPROM_MAGIC) {
			eepdata = (uint16_t *)eep;

			for (addr = 0; addr < SIZE_EEPROM_AR9287; addr++) {
				temp = bswap_16(*eepdata);
				*eepdata = temp;
				eepdata++;
			}
		} else {
			fprintf(stderr, "Invalid EEPROM Magic, endianness mismatch.\n");
			return false;
		}
	}

	if (!!(eep->baseEepHeader.eepMisc & AR5416_EEPMISC_BIG_ENDIAN) !=
	    edump->host_is_be)
		need_swap = true;

	if (need_swap)
		el = bswap_16(eep->baseEepHeader.length);
	else
		el = eep->baseEepHeader.length;

	if (el > sizeof(struct ar9287_eeprom))
		el = sizeof(struct ar9287_eeprom) / sizeof(uint16_t);
	else
		el = el / sizeof(uint16_t);

	eepdata = (uint16_t *)eep;

	for (i = 0; i < el; i++)
		sum ^= *eepdata++;

	if (need_swap) {
		uint32_t integer;
		uint16_t word;

		printf("EEPROM Endianness is not native.. Changing\n");

		word = bswap_16(eep->baseEepHeader.length);
		eep->baseEepHeader.length = word;

		word = bswap_16(eep->baseEepHeader.checksum);
		eep->baseEepHeader.checksum = word;

		word = bswap_16(eep->baseEepHeader.version);
		eep->baseEepHeader.version = word;

		word = bswap_16(eep->baseEepHeader.regDmn[0]);
		eep->baseEepHeader.regDmn[0] = word;

		word = bswap_16(eep->baseEepHeader.regDmn[1]);
		eep->baseEepHeader.regDmn[1] = word;

		word = bswap_16(eep->baseEepHeader.rfSilent);
		eep->baseEepHeader.rfSilent = word;

		word = bswap_16(eep->baseEepHeader.blueToothOptions);
		eep->baseEepHeader.blueToothOptions = word;

		word = bswap_16(eep->baseEepHeader.deviceCap);
		eep->baseEepHeader.deviceCap = word;

		integer = bswap_32(eep->modalHeader.antCtrlCommon);
		eep->modalHeader.antCtrlCommon = integer;

		for (i = 0; i < AR9287_MAX_CHAINS; i++) {
			integer = bswap_32(eep->modalHeader.antCtrlChain[i]);
			eep->modalHeader.antCtrlChain[i] = integer;
		}

		for (i = 0; i < AR_EEPROM_MODAL_SPURS; i++) {
			word = bswap_16(eep->modalHeader.spurChans[i].spurChan);
			eep->modalHeader.spurChans[i].spurChan = word;
		}
	}

	if (sum != 0xffff || eep_9287_get_ver(emp) != AR9287_EEP_VER ||
	    eep_9287_get_rev(emp) < AR5416_EEP_NO_BACK_VER) {
		fprintf(stderr, "Bad EEPROM checksum 0x%x or revision 0x%04x\n",
			sum, eep_9287_get_ver(emp));
		return false;
	}

	return true;
}

static void eep_9287_dump_base_header(struct edump *edump)
{
	struct eep_9287_priv *emp = edump->eepmap_priv;
	struct ar9287_eeprom *eep = &emp->eep;
	struct ar9287_base_eep_hdr *pBase = &eep->baseEepHeader;
	uint16_t i;

	EEP_PRINT_SECT_NAME("EEPROM Base Header");

	printf("%-30s : %2d\n", "Major Version",
	       pBase->version >> 12);
	printf("%-30s : %2d\n", "Minor Version",
	       pBase->version & 0xFFF);
	printf("%-30s : 0x%04X\n", "Checksum",
	       pBase->checksum);
	printf("%-30s : 0x%04X\n", "Length",
	       pBase->length);
	printf("%-30s : 0x%04X\n", "RegDomain1",
	       pBase->regDmn[0]);
	printf("%-30s : 0x%04X\n", "RegDomain2",
	       pBase->regDmn[1]);
	printf("%-30s : %02X:%02X:%02X:%02X:%02X:%02X\n",
	       "MacAddress",
	       pBase->macAddr[0], pBase->macAddr[1], pBase->macAddr[2],
	       pBase->macAddr[3], pBase->macAddr[4], pBase->macAddr[5]);
	printf("%-30s : 0x%04X\n",
	       "TX Mask", pBase->txMask);
	printf("%-30s : 0x%04X\n",
	       "RX Mask", pBase->rxMask);
	printf("%-30s : %d\n",
	       "OpFlags(5GHz)",
	       !!(pBase->opCapFlags & AR5416_OPFLAGS_11A));
	printf("%-30s : %d\n",
	       "OpFlags(2GHz)",
	       !!(pBase->opCapFlags & AR5416_OPFLAGS_11G));
	printf("%-30s : %d\n",
	       "OpFlags(Disable 2GHz HT20)",
	       !!(pBase->opCapFlags & AR5416_OPFLAGS_N_2G_HT20));
	printf("%-30s : %d\n",
	       "OpFlags(Disable 2GHz HT40)",
	       !!(pBase->opCapFlags & AR5416_OPFLAGS_N_2G_HT40));
	printf("%-30s : %d\n",
	       "OpFlags(Disable 5Ghz HT20)",
	       !!(pBase->opCapFlags & AR5416_OPFLAGS_N_5G_HT20));
	printf("%-30s : %d\n",
	       "OpFlags(Disable 5Ghz HT40)",
	       !!(pBase->opCapFlags & AR5416_OPFLAGS_N_5G_HT40));
	printf("%-30s : %d\n",
	       "Big Endian",
	       !!(pBase->eepMisc & AR5416_EEPMISC_BIG_ENDIAN));
	printf("%-30s : %d\n",
	       "Wake on Wireless",
	       !!(pBase->eepMisc & AR9287_EEPMISC_WOW));
	printf("%-30s : %d\n",
	       "Cal Bin Major Ver",
	       (pBase->binBuildNumber >> 24) & 0xFF);
	printf("%-30s : %d\n",
	       "Cal Bin Minor Ver",
	       (pBase->binBuildNumber >> 16) & 0xFF);
	printf("%-30s : %d\n",
	       "Cal Bin Build",
	       (pBase->binBuildNumber >> 8) & 0xFF);
	printf("%-30s : %d\n",
	       "OpenLoop PowerControl",
	       (pBase->openLoopPwrCntl & 0x1));

	if (eep_9287_get_rev(emp) >= AR5416_EEP_MINOR_VER_3) {
		printf("%-30s : %s\n",
		       "Device Type",
		       sDeviceType[(pBase->deviceType & 0x7)]);
	}

	printf("\nCustomer Data in hex:\n");
	for (i = 0; i < 64; i++) {
		printf("%02X ", eep->custData[i]);
		if ((i % 16) == 15)
			printf("\n");
	}

	printf("\n");
}

static void eep_9287_dump_modal_header(struct edump *edump)
{
#define PR(_token, _p, _val_fmt, _val)			\
	do {						\
		printf("%-23s %-2s", (_token), ":");	\
		printf("%s%"_val_fmt, _p, (_val));	\
		printf("\n");				\
	} while(0)

	struct eep_9287_priv *emp = edump->eepmap_priv;
	struct ar9287_eeprom *eep = &emp->eep;
	struct ar9287_modal_eep_hdr *pModal = &eep->modalHeader;

	EEP_PRINT_SECT_NAME("EEPROM Modal Header");

	PR("Chain0 Ant. Control", "0x", "X", pModal->antCtrlChain[0]);
	PR("Chain1 Ant. Control", "0x", "X", pModal->antCtrlChain[1]);
	PR("Ant. Common Control", "0x", "X", pModal->antCtrlCommon);
	PR("Chain0 Ant. Gain", "", "d", pModal->antennaGainCh[0]);
	PR("Chain1 Ant. Gain", "", "d", pModal->antennaGainCh[1]);
	PR("Switch Settle", "", "d", pModal->switchSettling);
	PR("Chain0 TxRxAtten", "", "d", pModal->txRxAttenCh[0]);
	PR("Chain1 TxRxAtten", "", "d", pModal->txRxAttenCh[1]);
	PR("Chain0 RxTxMargin", "", "d", pModal->rxTxMarginCh[0]);
	PR("Chain1 RxTxMargin", "", "d", pModal->rxTxMarginCh[1]);
	PR("ADC Desired size", "", "d", pModal->adcDesiredSize);
	PR("txEndToXpaOff", "", "d", pModal->txEndToXpaOff);
	PR("txEndToRxOn", "", "d", pModal->txEndToRxOn);
	PR("txFrameToXpaOn", "", "d", pModal->txFrameToXpaOn);
	PR("CCA Threshold)", "", "d", pModal->thresh62);
	PR("Chain0 NF Threshold", "", "d", pModal->noiseFloorThreshCh[0]);
	PR("Chain1 NF Threshold", "", "d", pModal->noiseFloorThreshCh[1]);
	PR("xpdGain", "", "d", pModal->xpdGain);
	PR("External PD", "", "d", pModal->xpd);
	PR("Chain0 I Coefficient", "", "d", pModal->iqCalICh[0]);
	PR("Chain1 I Coefficient", "", "d", pModal->iqCalICh[1]);
	PR("Chain0 Q Coefficient", "", "d", pModal->iqCalQCh[0]);
	PR("Chain1 Q Coefficient", "", "d", pModal->iqCalQCh[1]);
	PR("pdGainOverlap", "", "d", pModal->pdGainOverlap);
	PR("xPA Bias Level", "", "d", pModal->xpaBiasLvl);
	PR("txFrameToDataStart", "", "d", pModal->txFrameToDataStart);
	PR("txFrameToPaOn", "", "d", pModal->txFrameToPaOn);
	PR("HT40 Power Inc.", "", "d", pModal->ht40PowerIncForPdadc);
	PR("Chain0 bswAtten", "", "d", pModal->bswAtten[0]);
	PR("Chain1 bswAtten", "", "d", pModal->bswAtten[1]);
	PR("Chain0 bswMargin", "", "d", pModal->bswMargin[0]);
	PR("Chain1 bswMargin", "", "d", pModal->bswMargin[1]);
	PR("HT40 Switch Settle", "", "d", pModal->swSettleHt40);
	PR("AR92x7 Version", "", "d", pModal->version);
	PR("DriverBias1", "", "d", pModal->db1);
	PR("DriverBias2", "", "d", pModal->db1);
	PR("CCK OutputBias", "", "d", pModal->ob_cck);
	PR("PSK OutputBias", "", "d", pModal->ob_psk);
	PR("QAM OutputBias", "", "d", pModal->ob_qam);
	PR("PAL_OFF OutputBias", "", "d", pModal->ob_pal_off);

	printf("\n");
}

static void eep_9287_dump_power_info(struct edump *edump)
{
#define PR_TARGET_POWER(__pref, __field, __rates)			\
		EEP_PRINT_SUBSECT_NAME(__pref " per-rate target power");\
		ar5416_dump_target_power((void *)eep->__field,		\
				 ARRAY_SIZE(eep->__field),		\
				 __rates, ARRAY_SIZE(__rates), 1);	\
		printf("\n");

	struct eep_9287_priv *emp = edump->eepmap_priv;
	const struct ar9287_eeprom *eep = &emp->eep;

	EEP_PRINT_SECT_NAME("EEPROM Power Info");

	PR_TARGET_POWER("2 GHz CCK", calTargetPowerCck, eep_rates_cck);
	PR_TARGET_POWER("2 GHz OFDM", calTargetPower2G, eep_rates_ofdm);
	PR_TARGET_POWER("2 GHz HT20", calTargetPower2GHT20, eep_rates_ht);
	PR_TARGET_POWER("2 GHz HT40", calTargetPower2GHT40, eep_rates_ht);

#undef PR_TARGET_POWER
}

const struct eepmap eepmap_9287 = {
	.name = "9287",
	.desc = "AR9287 chip EEPROM map",
	.priv_data_sz = sizeof(struct eep_9287_priv),
	.fill_eeprom  = eep_9287_fill_eeprom,
	.check_eeprom = eep_9287_check_eeprom,
	.dump_base_header = eep_9287_dump_base_header,
	.dump_modal_header = eep_9287_dump_modal_header,
	.dump_power_info = eep_9287_dump_power_info,
};
