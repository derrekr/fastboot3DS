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
bool sdmmc_sd_read(u32 offset, u32 size, void *buf);
bool sdmmc_sd_write(u32 offset, u32 size, const void *buf);
bool sdmmc_sd_close(void);
bool sdmmc_sd_is_active(void);

static dev_struct dev_sd = {
	"sd",
	0,
	&sdmmc_sd_init,
	&sdmmc_sd_read,
	&sdmmc_sd_write,
	&sdmmc_sd_close,
	&sdmmc_sd_is_active
};
const dev_struct *dev_sdcard = &dev_sd;

// Raw NAND device
bool sdmmc_rnand_init(void);
bool sdmmc_rnand_read(u32 offset, u32 size, void *buf);
bool sdmmc_rnand_write(u32 offset, u32 size, const void *buf);
bool sdmmc_rnand_close(void);
bool sdmmc_rnand_is_active(void);

static dev_struct dev_rnand = {
	"rnand",
	0,
	&sdmmc_rnand_init,
	&sdmmc_rnand_read,
	&sdmmc_rnand_write,
	&sdmmc_rnand_close,
	&sdmmc_rnand_is_active
};
const dev_struct *dev_rawnand = &dev_rnand;

// Decrypted NAND device
typedef struct {
	u32 offset;
	u32 size;
	u8 type;
	u8 keyslot;
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
bool sdmmc_dnand_read(u32 offset, u32 size, void *buf);
bool sdmmc_dnand_write(u32 offset, u32 size, const void *buf);
bool sdmmc_dnand_close(void);
bool sdmmc_dnand_is_active(void);

static dev_dnand_struct dev_dnand = {
	{
		"dnand",
		0,
		&sdmmc_dnand_init,
		&sdmmc_dnand_read,
		&sdmmc_dnand_write,
		&sdmmc_dnand_close,
		&sdmmc_dnand_is_active
	}
};
const dev_struct *dev_decnand = &dev_dnand.dev;

// wifi flash device
bool wififlash_init(void);
bool wififlash_read(u32 offset, u32 size, void *buf);
bool wififlash_close(void);
bool wififlash_is_active(void);

static dev_struct dev_wififlash = {
	"wififlash",
	0,
	&wififlash_init,
	&wififlash_read,
	0,
	&wififlash_close,
	&wififlash_is_active
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

bool sdmmc_sd_read(u32 offset, u32 size, void *buf)
{
	if(!dev_sd.initialized)
		return false;
	
	return !sdmmc_sdcard_readsectors(offset >> 9, size >> 9, buf);
}

bool sdmmc_sd_write(u32 offset, u32 size, const void *buf)
{
	if(!dev_sd.initialized)
		return false;
	
	return !sdmmc_sdcard_writesectors(offset >> 9, size >> 9, buf);
}

bool sdmmc_sd_close(void)
{
	return true;
}

bool sdmmc_sd_is_active(void)
{
	return (sdmmc_read16(REG_SDSTATUS0) & TMIO_STAT0_SIGSTATE);
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

bool sdmmc_rnand_read(u32 offset, u32 size, void *buf)
{
	if(!dev_rnand.initialized)
		return false;
	
	return !sdmmc_nand_readsectors(offset >> 9, size >> 9, buf);
}

bool sdmmc_rnand_write(u32 offset, u32 size, const void *buf)
{
	if(!dev_rnand.initialized)
		return false;
	
	return !sdmmc_nand_writesectors(offset >> 9, size >> 9, buf);
}

bool sdmmc_rnand_close(void)
{
	return true;
}

bool sdmmc_rnand_is_active(void)
{
	return dev_rnand.initialized;
}

// ------------------------------ decrypted nand glue functions ------------------------------
bool sdmmc_dnand_init(void)
{
	Ncsd_header header;
	u32 hash[8];
	u32 twlKeyX[4]; // TWL keys
	u32 twlKeyY[4];
	extern bool unit_is_new3ds;
	extern u32 ctr_nand_offset;


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
			dev_dnand.partitions[i].offset = header.part[i].offset * 0x200;
			dev_dnand.partitions[i].size   = header.part[i].size * 0x200;
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
						ctr_nand_offset = header.part[i].offset * 0x200;
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
		sha((void*)0x01FFCD84, 16, hash, SHA_INPUT_BIG | SHA_MODE_1, SHA_OUTPUT_BIG);
		memcpy(dev_dnand.twlCounter, hash, 16);
		sha((void*)0x01FFCD84, 16, hash, SHA_INPUT_BIG | SHA_MODE_256, SHA_OUTPUT_LITTLE);
		memcpy(dev_dnand.ctrCounter, hash, 16);

		// TWL keyslot 0x03 keyX
		twlKeyX[0] = (*((u32*)0x01FFB808) ^ 0xB358A6AF) | 0x80000000;
		twlKeyX[1] = 0x544E494E; // "NINT"
		twlKeyX[2] = 0x4F444E45; // "ENDO"
		twlKeyX[3] = *((u32*)0x01FFB80C) ^ 0x08C267B7;
		AES_setTwlKeyX(AES_INPUT_LITTLE | AES_INPUT_REVERSED_ORDER, 3, twlKeyX);

		// TWL keyslot 0x03 keyY
		for(int i = 0; i < 3; i++) twlKeyY[i] = ((u32*)0x01FFD3C8)[i];
		twlKeyY[3] = 0xE1A00005;
		AES_setTwlKeyY(AES_INPUT_LITTLE | AES_INPUT_REVERSED_ORDER, 3, twlKeyY);

		// Crypt settings
		AES_setCryptParams(&dev_dnand.twlAesCtx, AES_FLUSH_READ_FIFO | AES_FLUSH_WRITE_FIFO | AES_BIT12 | AES_BIT13 | AES_OUTPUT_LITTLE |
							AES_INPUT_LITTLE | AES_OUTPUT_REVERSED_ORDER | AES_INPUT_REVERSED_ORDER | AES_MODE_CTR);
		AES_setCryptParams(&dev_dnand.ctrAesCtx, AES_FLUSH_READ_FIFO | AES_FLUSH_WRITE_FIFO | AES_BIT12 | AES_BIT13 | AES_OUTPUT_BIG |
							AES_INPUT_BIG | AES_OUTPUT_NORMAL_ORDER | AES_INPUT_NORMAL_ORDER | AES_MODE_CTR);

		dev_dnand.dev.initialized = true;
	}
	return true;
}

nand_partition_struct *find_partition(u32 offset, u32 size)
{
	for(int i=0; i<8; i++)
	{
		nand_partition_struct *partition = &dev_dnand.partitions[i];
		if((partition->offset <= offset) && (partition->size >= size)
			&& (partition->offset + partition->size >= offset + size))
		return partition;
	}
	return NULL;
}

bool sdmmc_dnand_read(u32 offset, u32 size, void *buf)
{
	//printf("Decnand read: offset: 0x%X, size: 0x%X\n", (unsigned int)offset, (unsigned int)size);
	if(!dev_dnand.dev.initialized) return false;

	AES_ctx *ctx;
	nand_partition_struct *partition = find_partition(offset, size);


	if(!partition) return false;
	if(partition->keyslot == 0xFF) return false;	// unknown partition type
	
	AES_selectKeyslot(partition->keyslot);
	if(partition->keyslot == 0x03)
	{
		ctx = &dev_dnand.twlAesCtx;
		AES_setCtrIvNonce(ctx, dev_dnand.twlCounter, AES_INPUT_LITTLE | AES_INPUT_REVERSED_ORDER | AES_MODE_CTR, offset);
	}
	else
	{
		ctx = &dev_dnand.ctrAesCtx;
		AES_setCtrIvNonce(ctx, dev_dnand.ctrCounter, AES_INPUT_LITTLE | AES_INPUT_NORMAL_ORDER | AES_MODE_CTR, offset);
	}
	
	if(sdmmc_nand_readsectors(offset>>9, size>>9, buf)) return false;
	AES_crypt(ctx, buf, buf, size);

	return true;
}

bool sdmmc_dnand_write(u32 offset, u32 size, const void *buf)
{
	if(!dev_dnand.dev.initialized)
		return false;
	
	//return !sdmmc_nand_writesectors(offset >> 9, size >> 9, buf);
	printf("Decnand write not implemented!\n");
	return false;
}

bool sdmmc_dnand_close(void)
{
	return true;
}

bool sdmmc_dnand_is_active(void)
{
	return sdmmc_rnand_is_active();
}


// ------------------------------ wifi flash glue functions ------------------------------

bool wififlash_init(void)
{
	if(dev_wififlash.initialized) return true;
	if(!spiflash_get_status()) return false;
	dev_wififlash.initialized = true;
	return true;
}

bool wififlash_read(u32 offset, u32 size, void *buf)
{
	if(!dev_wififlash.initialized) return false;
	spiflash_read(offset, size, buf);
	return true;
}

bool wififlash_close(void)
{
	// nothing to do here..?
	return true;
}

bool wififlash_is_active(void)
{
	if(dev_wififlash.initialized) return true;
	return wififlash_init();
}
