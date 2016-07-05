#include <stdio.h>
#include <string.h>
#include "types.h"
#include "mem_map.h"
#include "arm9/ncsd.h"
#include "arm9/sdmmc.h"
#include "arm9/spiflash.h"
#include "arm9/crypto.h"
#include "arm9/ndma.h"
#include "arm9/dev.h"



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
	u32 sector;
	u32 count;
	u8  type;
	u8  keyslot;
} nand_partition_struct;

typedef struct {
	dev_struct dev;
	u32 twlCounter[4];
	u32 ctrCounter[4];
	AES_ctx twlAesCtx;
	AES_ctx ctrAesCtx;
	nand_partition_struct partitions[8];
} dev_dnand_struct;

bool sdmmc_dnand_init(void);
bool sdmmc_dnand_read_sector(u32 sector, u32 count, void *buf);
bool sdmmc_dnand_write_sector(u32 sector, u32 count, const void *buf);
bool sdmmc_dnand_close(void);
bool sdmmc_dnand_is_active(void);

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
};
const dev_struct *dev_decnand = &dev_dnand.dev;


// wifi flash device
bool nvram_init(void);
bool nvram_read_sector(u32 sector, u32 count, void *buf);
bool nvram_close(void);
bool nvram_is_active(void);
u32  nvram_get_sector_count(void);

dev_struct dev_wififlash = {
	"nvram",
	false,
	nvram_init,
	nvram_read_sector,
	NULL,
	nvram_close,
	nvram_is_active,
	nvram_get_sector_count
};
const dev_struct *dev_flash = &dev_wififlash;


// -------------------------------- sd card glue functions --------------------------------
bool sdmmc_sd_init(void)
{
	if(!dev_rnand.initialized && !dev_sd.initialized && !dev_dnand.dev.initialized)
		sdmmc_init();
	
	if(!dev_sd.initialized)
	{
		// thanks yellows8
		*((u16*)0x10000020) |= 0x200; //If not set, the hardware will not detect any inserted card on the sdbus.
		*((u16*)0x10000020) &= ~0x1; //If set while bitmask 0x200 is set, a sdbus command timeout error will occur during sdbus init.
		if(SD_Init()) return false;
		dev_sd.initialized = true;
	}
	return true;
}

bool sdmmc_sd_read_sector(u32 sector, u32 count, void *buf)
{
	return !sdmmc_sdcard_readsectors(sector, count, buf);
}

bool sdmmc_sd_write_sector(u32 sector, u32 count, const void *buf)
{
	return !sdmmc_sdcard_writesectors(sector, count, buf);
}

bool sdmmc_sd_close(void)
{
	dev_sd.initialized = false;
	return true;
}

bool sdmmc_sd_is_active(void)
{
	return (sdmmc_read16(REG_SDSTATUS0) & TMIO_STAT0_SIGSTATE);
}

u32 sdmmc_sd_get_sector_count(void)
{
	return getMMCDevice(1)->total_size;
}


// -------------------------------- raw nand glue functions --------------------------------
bool sdmmc_rnand_init(void)
{
	if(!dev_rnand.initialized && !dev_sd.initialized && !dev_dnand.dev.initialized)
		sdmmc_init();
	
	if(!dev_rnand.initialized && !dev_dnand.dev.initialized) {
		if(Nand_Init()) return false;
		dev_rnand.initialized = true;
	}
	return true;
}

bool sdmmc_rnand_read_sector(u32 sector, u32 count, void *buf)
{
	return !sdmmc_nand_readsectors(sector, count, buf);
}

bool sdmmc_rnand_write_sector(u32 sector, u32 count, const void *buf)
{
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
	return getMMCDevice(0)->total_size;
}


// ------------------------------ decrypted nand glue functions ------------------------------
bool sdmmc_dnand_init(void)
{
	NCSD_header header;
	u32 hash[8];
	u32 twlKeyX[4]; // TWL keys
	u32 twlKeyY[4];
	extern bool unit_is_new3ds;
	extern u32 ctr_nand_sector;


	if(!dev_rnand.initialized && !dev_sd.initialized && !dev_dnand.dev.initialized)
		sdmmc_init();

	if(!dev_dnand.dev.initialized)
	{
		if(!dev_rnand.initialized)
		{
			Nand_Init();
			dev_rnand.initialized = true;
		}

		// Read NCSD header
		if(sdmmc_nand_readsectors(0, 1, (void*)&header)) return false;

		// Check "NCSD" magic
		if(header.magic != 0x4453434E) return false;

		// Collect partition infos...
		for(int i = 0; i < 8; i++)
		{
			dev_dnand.partitions[i].sector = header.partitions[i].mediaOffset;
			dev_dnand.partitions[i].count  = header.partitions[i].mediaSize;
			dev_dnand.partitions[i].type   = header.partFsType[i];

			switch(dev_dnand.partitions[i].type)
			{
				case 1:
					if(i == 0) dev_dnand.partitions[i].keyslot = 0x03; // TWL NAND partition
					if(i == 4)                                         // CTR NAND partition
					{
						if(unit_is_new3ds) dev_dnand.partitions[i].keyslot = 0x05; // TODO: Load N3DS keyY
						else dev_dnand.partitions[i].keyslot = 0x04;
						// Set CTR NAND partition offset for diskio.c
						ctr_nand_sector = header.partitions[i].mediaOffset;
					}
					break;
				case 3: // firmX
					dev_dnand.partitions[i].keyslot = 0x06;
					break;
				case 4: // AGB_FIRM savegame
					dev_dnand.partitions[i].keyslot = 0x07;
					break;
				default: // Unused
					dev_dnand.partitions[i].keyslot = 0xFF;
			}
		}

		// Hash NAND CID to create the CTRs for crypto
		sha((u32*)0x01FFCD84, 16, hash, SHA_INPUT_BIG | SHA_MODE_1, SHA_OUTPUT_BIG);
		memcpy(dev_dnand.twlCounter, hash, 16);
		sha((u32*)0x01FFCD84, 16, hash, SHA_INPUT_BIG | SHA_MODE_256, SHA_OUTPUT_LITTLE);
		memcpy(dev_dnand.ctrCounter, hash, 16);

		// TWL keyslot 0x03 keyX
		twlKeyX[0] = (*((u32*)0x01FFB808) ^ 0xB358A6AF) | 0x80000000;
		twlKeyX[1] = 0x544E494E; // "NINT"
		twlKeyX[2] = 0x4F444E45; // "ENDO"
		twlKeyX[3] = *((u32*)0x01FFB80C) ^ 0x08C267B7;
		AES_setKey(AES_INPUT_LITTLE | AES_INPUT_REVERSED_ORDER, 3, AES_KEY_TYPE_X, twlKeyX, false, false);

		// TWL keyslot 0x03 keyY
		for(int i = 0; i < 3; i++) twlKeyY[i] = ((u32*)0x01FFD3C8)[i];
		twlKeyY[3] = 0xE1A00005;
		AES_setKey(AES_INPUT_LITTLE | AES_INPUT_REVERSED_ORDER, 3, AES_KEY_TYPE_Y, twlKeyY, false, true);

		// Crypt settings
		AES_setCryptParams(&dev_dnand.twlAesCtx, AES_BIT12 | AES_BIT13 | AES_OUTPUT_LITTLE | AES_INPUT_LITTLE |
							AES_OUTPUT_REVERSED_ORDER | AES_INPUT_REVERSED_ORDER | AES_MODE_CTR);
		AES_setCryptParams(&dev_dnand.ctrAesCtx, AES_BIT12 | AES_BIT13 | AES_OUTPUT_BIG | AES_INPUT_BIG |
							AES_OUTPUT_NORMAL_ORDER | AES_INPUT_NORMAL_ORDER | AES_MODE_CTR);

		dev_dnand.dev.initialized = true;
	}

	return true;
}

static nand_partition_struct *find_partition(u32 sector, u32 count)
{
	for(u32 i = 0; i < 8; i++)
	{
		nand_partition_struct *partition = &dev_dnand.partitions[i];
		if((partition->sector <= sector) && (partition->count >= count)
			&& (partition->sector + partition->count >= sector + count))
		return partition;
	}

	return NULL;
}

bool sdmmc_dnand_read_sector(u32 sector, u32 count, void *buf)
{
	if(!dev_dnand.dev.initialized) return false;

	nand_partition_struct *partition = find_partition(sector, count);


	if(!partition) return false;
	if(partition->keyslot == 0xFF) return false;	// unknown partition type

	AES_ctx *ctx;
	AES_selectKeyslot(partition->keyslot, true);
	if(partition->keyslot == 0x03)
	{
		ctx = &dev_dnand.twlAesCtx;
		AES_setCtrIvNonce(ctx, dev_dnand.twlCounter, AES_INPUT_LITTLE | AES_INPUT_REVERSED_ORDER | AES_MODE_CTR, sector<<9);
	}
	else
	{
		ctx = &dev_dnand.ctrAesCtx;
		AES_setCtrIvNonce(ctx, dev_dnand.ctrCounter, AES_INPUT_LITTLE | AES_INPUT_NORMAL_ORDER | AES_MODE_CTR, sector<<9);
	}
	
	if(sdmmc_nand_readsectors(sector, count, buf)) return false;
	AES_crypt(ctx, buf, buf, count<<9);

	return true;
}

bool sdmmc_dnand_write_sector(u32 sector, u32 count, const void *buf)
{
	if(!dev_dnand.dev.initialized) return false;

	//return !sdmmc_nand_writesectors(sector, count, buf);
	printf("Decnand write not implemented!\n");

	return false;
}

bool sdmmc_dnand_close(void)
{
	dev_dnand.dev.initialized = false;
	return true;
}

bool sdmmc_dnand_is_active(void)
{
	return sdmmc_rnand_is_active();
}


// ------------------------------ wifi flash glue functions ------------------------------

bool nvram_init(void)
{
	if(dev_wififlash.initialized) return true;
	if(!spiflash_get_status()) return false;
	dev_wififlash.initialized = true;
	return true;
}

bool nvram_read_sector(u32 sector, u32 count, void *buf)
{
	if(!dev_wififlash.initialized) return false;
	spiflash_read(sector<<9, count<<9, buf);
	return true;
}

bool nvram_close(void)
{
	// nothing to do here..?
	dev_wififlash.initialized = false;
	return true;
}

bool nvram_is_active(void)
{
	if(dev_wififlash.initialized) return true;
	return nvram_init();
}

u32 nvram_get_sector_count(void)
{
	return 0x20000>>9;
}
