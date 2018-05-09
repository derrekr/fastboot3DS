/*
 *   This file is part of fastboot 3DS
 *   Copyright (C) 2017 derrek, profi200
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "fb_assert.h"
#include "mem_map.h"
#include "arm9/ncsd.h"
#include "arm9/hardware/sdmmc.h"
#include "arm9/hardware/cfg9.h"
#include "arm9/hardware/spiflash.h"
#include "arm9/hardware/interrupt.h"
#include "fs.h"
#include "arm9/hardware/crypto.h"
#include "hardware/cache.h"
#include "arm9/hardware/timer.h"
#include "util.h"
#include "arm9/dev.h"
#include "arm9/partitions.h"


// SD card device
bool sdmmc_sd_init(void);
bool sdmmc_sd_read_sector(u32 sector, u32 count, void *buf);
bool sdmmc_sd_write_sector(u32 sector, u32 count, const void *buf);
bool sdmmc_sd_close(void);
bool sdmmc_sd_is_active(void);
u32  sdmmc_sd_get_sector_count(void);

static dev_struct dev_sd = {
	"sd",
	false,
	sdmmc_sd_init,
	sdmmc_sd_read_sector,
	sdmmc_sd_write_sector,
	sdmmc_sd_close,
	sdmmc_sd_is_active,
	sdmmc_sd_get_sector_count
};
const dev_struct *dev_sdcard = &dev_sd;


// Raw NAND device
bool sdmmc_rnand_init(void);
bool sdmmc_rnand_read_sector(u32 sector, u32 count, void *buf);
bool sdmmc_rnand_write_sector(u32 sector, u32 count, const void *buf);
bool sdmmc_rnand_close(void);
bool sdmmc_rnand_is_active(void);
u32  sdmmc_rnand_get_sector_count(void);

static dev_struct dev_rnand = {
	"rnand",
	false,
	sdmmc_rnand_init,
	sdmmc_rnand_read_sector,
	sdmmc_rnand_write_sector,
	sdmmc_rnand_close,
	sdmmc_rnand_is_active,
	sdmmc_rnand_get_sector_count
};
const dev_struct *dev_rawnand = &dev_rnand;


// Decrypted NAND device
typedef struct {
	dev_struct dev;
	u32 twlCounter[4];
	u32 ctrCounter[4];
	AES_ctx twlAesCtx;
	AES_ctx ctrAesCtx;
} dev_dnand_struct;

bool sdmmc_dnand_init(void);
bool sdmmc_dnand_read_sector(u32 sector, u32 count, void *buf);
bool sdmmc_dnand_write_sector(u32 sector, u32 count, const void *buf);
bool sdmmc_dnand_close(void);
bool sdmmc_dnand_is_active(void);

// gcc throws a bullshit warning about missing braces here.
// Seems to be https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53119
static dev_dnand_struct dev_dnand = {
	{
		"dnand",
		false,
		sdmmc_dnand_init,
		sdmmc_dnand_read_sector,
		sdmmc_dnand_write_sector,
		sdmmc_dnand_close,
		sdmmc_dnand_is_active,
		NULL
	},
	{0},
	{0},
	{0},
	{0}
};
const dev_struct *dev_decnand = &dev_dnand.dev;


// wifi flash device
bool wififlash_init(void);
bool wififlash_read_sector(u32 sector, u32 count, void *buf);
bool wififlash_close(void);
bool wififlash_is_active(void);
u32  wififlash_get_sector_count(void);

dev_struct dev_wififlash = {
	"nvram",
	false,
	wififlash_init,
	wififlash_read_sector,
	NULL,
	wififlash_close,
	wififlash_is_active,
	wififlash_get_sector_count
};
const dev_struct *dev_flash = &dev_wififlash;


static void sdioHandler(UNUSED u32 id);

// -------------------------------- sd card glue functions --------------------------------
bool sdmmc_sd_init(void)
{
	if(!dev_sd.initialized)
	{
		sdmmc_dnand_close();
		sdmmc_rnand_close();
		sdmmc_init();

		// thanks yellows8
		*((vu16*)0x10000020) = (*((vu16*)0x10000020) & ~0x1u) | 0x200u;

		u32 timeout = 34; // In ms. 33 works. 34 for safety.

		do {
			// if sd card is ready, stop polling
			if(sdmmc_read16(REG_SDSTATUS0) & TMIO_STAT0_SIGSTATE)
				break;

			TIMER_sleep(2);
			timeout -= 2;
		} while(timeout);
		
		if(!timeout)	// we timed out
			return false;

		if(SD_Init()) return false;
		dev_sd.initialized = true;
		IRQ_registerHandler(IRQ_SDIO_1, sdioHandler);

		for(FsDrive i = FS_DRIVE_TWLN; i <= FS_DRIVE_NAND; i++)
		{
			if(fIsDriveMounted(i))
			{
				fUnmount(i);
				fMount(i);
			}
		}
	}

	return true;
}

static void sdioHandler(UNUSED u32 id)
{
	// Hacky way to detect SD pulls. We need a proper MMC driver.
	if(!(sdmmc_read16(REG_SDSTATUS0) & TMIO_STAT0_SIGSTATE))
	{
		const u32 oldState = enterCriticalSection();

		sdmmc_sd_close();
		fUnmount(FS_DRIVE_SDMC);

		leaveCriticalSection(oldState);
	}
}

bool sdmmc_sd_read_sector(u32 sector, u32 count, void *buf)
{
	if(!dev_sd.initialized) return false;

	fb_assert(count != 0);
	fb_assert(buf != NULL);

	return !sdmmc_sdcard_readsectors(sector, count, buf);
}

bool sdmmc_sd_write_sector(u32 sector, u32 count, const void *buf)
{
	if(!dev_sd.initialized) return false;

	fb_assert(count != 0);
	fb_assert(buf != NULL);

	return !sdmmc_sdcard_writesectors(sector, count, buf);
}

bool sdmmc_sd_close(void)
{
	dev_sd.initialized = false;
	return true;
}

bool sdmmc_sd_is_active(void)
{
	return (sdmmc_read16(REG_SDSTATUS0) & TMIO_STAT0_SIGSTATE) && dev_sd.initialized;
}

u32 sdmmc_sd_get_sector_count(void)
{
	if(!dev_sd.initialized) return 0;
	return getMMCDevice(1)->total_size;
}


// -------------------------------- raw nand glue functions --------------------------------
bool sdmmc_rnand_init(void)
{	
	if(!dev_rnand.initialized)
	{
		// sdmmc_sd_init() calls sdmmc_init() for us so if it fails
		// we try to init NAND anyway.
		if(!dev_sd.initialized) sdmmc_sd_init();

		if(Nand_Init()) return false;
		dev_rnand.initialized = true;
	}

	return true;
}

bool sdmmc_rnand_read_sector(u32 sector, u32 count, void *buf)
{
	if(!dev_rnand.initialized) return false;

	fb_assert(count != 0);
	fb_assert(buf != NULL);

	return !sdmmc_nand_readsectors(sector, count, buf);
}

bool sdmmc_rnand_write_sector(u32 sector, u32 count, const void *buf)
{
	if(!dev_rnand.initialized) return false;

	fb_assert(count != 0);
	fb_assert(buf != NULL);

	return !sdmmc_nand_writesectors(sector, count, buf);
}

bool sdmmc_rnand_close(void)
{
	dev_rnand.initialized = false;
	return true;
}

bool sdmmc_rnand_is_active(void)
{
	return dev_rnand.initialized;
}

u32 sdmmc_rnand_get_sector_count(void)
{
	if(!dev_rnand.initialized) return 0;
	return getMMCDevice(0)->total_size;
}


// ------------------------------ decrypted nand glue functions ------------------------------
bool sdmmc_dnand_init(void)
{
	if(!dev_dnand.dev.initialized)
	{
		partitionsReset();
		if(!dev_rnand.initialized)
		{
			if(!sdmmc_rnand_init()) return false;
		}

		NCSD_header header;
		size_t temp;
		extern u32 ctr_nand_sector;


		// Read NCSD header
		if(sdmmc_nand_readsectors(0, 1, (void*)&header)) return false;

		// Check "NCSD" magic
		if(memcmp(&header.magic, "NCSD", 4) != 0) return false;

		// Collect partition info...
		for(int i = 0; i < MAX_PARTITIONS; i++)
		{
			u8 type = header.partFsType[i];
			size_t sector = header.partitions[i].mediaOffset;
			size_t index = partitionAdd(sector, header.partitions[i].mediaSize, type);

			switch(type)
			{
				case 1:
					if(i == 0)
					{
						partitionSetKeyslot(index, 0x03); // TWL NAND partition
						partitionSetName(index, "twln");
					}
					else if(i == 4)	// CTR NAND partition
					{
						if(REG_CFG9_SOCINFO & 2) // New 3DS
							partitionSetKeyslot(index, 0x05);
						else
							partitionSetKeyslot(index, 0x04);
						
						partitionSetName(index, "nand");
						
						// Set CTR NAND partition offset for diskio.c
						ctr_nand_sector = sector;
					}
					break;
				case 3: // firmX
					/* NOTE: This assumes there's not more than two firmware partitions! */
					partitionSetKeyslot(index, 0x06);
					if(partitionGetIndex("firm0", &temp))
						partitionSetName(index, "firm1");
					else
						partitionSetName(index, "firm0");
					break;
				case 4: // AGB_FIRM savegame
					partitionSetKeyslot(index, 0x07);
					partitionSetName(index, "agb");
					break;
				default: // Unused
					partitionSetKeyslot(index, 0xFF);
					partitionSetName(index, "invalid");
			}
		}

		// Hash NAND CID to create the CTRs for crypto
		u32 cid[4];
		if(sdmmc_get_cid(true, cid)) return false;
		sha(cid, 16, dev_dnand.twlCounter, SHA_INPUT_BIG | SHA_MODE_1, SHA_OUTPUT_BIG);
		sha(cid, 16, dev_dnand.ctrCounter, SHA_INPUT_BIG | SHA_MODE_256, SHA_OUTPUT_LITTLE);

		// Crypt settings
		AES_setCryptParams(&dev_dnand.twlAesCtx, AES_INPUT_LITTLE | AES_INPUT_REVERSED,
		                   AES_OUTPUT_LITTLE | AES_OUTPUT_REVERSED);
		AES_setCryptParams(&dev_dnand.ctrAesCtx, AES_INPUT_BIG | AES_INPUT_NORMAL,
		                   AES_OUTPUT_BIG | AES_OUTPUT_NORMAL);

		dev_dnand.dev.initialized = true;
	}

	return true;
}

bool sdmmc_dnand_read_sector(u32 sector, u32 count, void *buf)
{
	if(!dev_dnand.dev.initialized) return false;

	size_t index;
	u8 keyslot;

	fb_assert(count != 0);
	fb_assert(buf != NULL);

	if(!partitionFind(sector, count, &index)) return false;
		
	partitionGetKeyslot(index, &keyslot);
	if(keyslot == 0xFF) return false; // unknown partition type

	AES_ctx *ctx;
	AES_selectKeyslot(keyslot);
	if(keyslot == 0x03)
	{
		ctx = &dev_dnand.twlAesCtx;
		AES_setCtrIv(ctx, AES_INPUT_LITTLE | AES_INPUT_REVERSED, dev_dnand.twlCounter);
		AES_addCounter(ctx->ctrIvNonce, sector<<9);
	}
	else
	{
		ctx = &dev_dnand.ctrAesCtx;
		AES_setCtrIv(ctx, AES_INPUT_LITTLE | AES_INPUT_NORMAL, dev_dnand.ctrCounter);
		AES_addCounter(ctx->ctrIvNonce, sector<<9);
	}
	
	if(sdmmc_nand_readsectors(sector, count, buf)) return false;
	flushInvalidateDCacheRange(buf, count<<9);
	AES_ctr(ctx, buf, buf, count<<5, true);

	return true;
}

bool sdmmc_dnand_write_sector(u32 sector, u32 count, const void *buf)
{
	if(!dev_dnand.dev.initialized) return false;

	size_t index;
	u8 keyslot;

	fb_assert(count != 0);
	fb_assert(buf != NULL);

	if(!partitionFind(sector, count, &index)) return false;

	partitionGetKeyslot(index, &keyslot);
	if(keyslot == 0xFF) return false; // unknown partition type

	const size_t crypto_sec_size = min(count, 0x1000>>9);
	void *crypto_buf = malloc(crypto_sec_size<<9);
	if(!crypto_buf)
		return false;

	flushDCacheRange(buf, count<<9);

	AES_selectKeyslot(keyslot);
	AES_ctx *ctx;
	if(keyslot == 0x03)
	{
		ctx = &dev_dnand.twlAesCtx;
		AES_setCtrIv(ctx, AES_INPUT_LITTLE | AES_INPUT_REVERSED, dev_dnand.twlCounter);
		AES_addCounter(ctx->ctrIvNonce, sector<<9);
	}
	else
	{
		ctx = &dev_dnand.ctrAesCtx;
		AES_setCtrIv(ctx, AES_INPUT_LITTLE | AES_INPUT_NORMAL, dev_dnand.ctrCounter);
		AES_addCounter(ctx->ctrIvNonce, sector<<9);
	}
	
	do {
		size_t crypt_size = min(count, crypto_sec_size);

		invalidateDCacheRange(crypto_buf, crypt_size<<9);
		AES_ctr(ctx, buf, crypto_buf, crypt_size<<5, true);
		if(sdmmc_nand_writesectors(sector, crypt_size, crypto_buf))
		{
			free(crypto_buf);
			return false;
		}

		sector += crypt_size;
		count -= crypt_size;
		buf += crypt_size<<9;
	} while(count);

	free(crypto_buf);

	return true;
}

bool sdmmc_dnand_close(void)
{
	dev_dnand.dev.initialized = false;
	return true;
}

bool sdmmc_dnand_is_active(void)
{
	return sdmmc_rnand_is_active() && dev_dnand.dev.initialized;
}


// ------------------------------ wifi flash glue functions ------------------------------

bool wififlash_init(void)
{
	if(dev_wififlash.initialized) return true;
	if(!spiflash_get_status()) return false;
	dev_wififlash.initialized = true;
	return true;
}

bool wififlash_read_sector(u32 sector, u32 count, void *buf)
{
	if(!dev_wififlash.initialized) return false;
	spiflash_read(sector<<9, count<<9, buf);
	return true;
}

bool wififlash_close(void)
{
	dev_wififlash.initialized = false;
	return true;
}

bool wififlash_is_active(void)
{
	return dev_wififlash.initialized;
}

u32 wififlash_get_sector_count(void)
{
	if(!dev_wififlash.initialized) return 0;
	return 0x20000>>9;
}
