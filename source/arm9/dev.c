#include <stdio.h>
#include "types.h"
#include "mem_map.h"
#include "arm9/sdmmc.h"
#include "arm9/spiflash.h"
#include "arm9/crypto.h"
#include "arm9/ndma.h"
#include "arm9/dev.h"

// SD card device
bool sdmmc_sd_init();
bool sdmmc_sd_read(u32 offset, u32 size, void *buf);
bool sdmmc_sd_write(u32 offset, u32 size, void *buf);
bool sdmmc_sd_close();
bool sdmmc_sd_is_active();

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
bool sdmmc_rnand_init();
bool sdmmc_rnand_read(u32 offset, u32 size, void *buf);
bool sdmmc_rnand_write(u32 offset, u32 size, void *buf);
bool sdmmc_rnand_close();
bool sdmmc_rnand_is_active();

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
	u32 cid_hash[8];	// NAND CID hash
	AES_ctx aes_ctx;
	nand_partition_struct partitions[8];
} dev_dnand_struct;

bool sdmmc_dnand_init();
bool sdmmc_dnand_read(u32 offset, u32 size, void *buf);
bool sdmmc_dnand_write(u32 offset, u32 size, void *buf);
bool sdmmc_dnand_close();
bool sdmmc_dnand_is_active();

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
bool wififlash_init();
bool wififlash_read(u32 offset, u32 size, void *buf);
bool wififlash_close();
bool wififlash_is_active();

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
bool sdmmc_sd_init()
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

bool sdmmc_sd_write(u32 offset, u32 size, void *buf)
{
	if(!dev_sd.initialized)
		return false;
	
	return !sdmmc_sdcard_writesectors(offset >> 9, size >> 9, buf);
}

bool sdmmc_sd_close()
{
	return true;
}

bool sdmmc_sd_is_active()
{
	return (sdmmc_read16(REG_SDSTATUS0) & TMIO_STAT0_SIGSTATE);
}

// -------------------------------- raw nand glue functions --------------------------------
bool sdmmc_rnand_init()
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

bool sdmmc_rnand_write(u32 offset, u32 size, void *buf)
{
	if(!dev_rnand.initialized)
		return false;
	
	return !sdmmc_nand_writesectors(offset >> 9, size >> 9, buf);
}

bool sdmmc_rnand_close()
{
	return true;
}

bool sdmmc_rnand_is_active()
{
	return dev_rnand.initialized;
}

// ------------------------------ decrypted nand glue functions ------------------------------
bool sdmmc_dnand_init()
{
	u8 sector[0x200];
	const char *name;
	const u32 magic = 0x4453434E; // 'NCSD'
	
	if(!dev_rnand.initialized && !dev_sd.initialized && !dev_dnand.dev.initialized)
		sdmmc_init();
	
	if(!dev_dnand.dev.initialized) {
		if(!dev_rnand.initialized) {
			Nand_Init();
			dev_rnand.initialized = true;
		}
		
		// this is needed for decryption
		sha((void*)0x01FFCD84, 16, (u8*)dev_dnand.cid_hash, SHA_INPUT_BIG | SHA_MODE_256, SHA_OUTPUT_LITTLE);
		
		if(sdmmc_nand_readsectors(0, 1, sector)) return false;	// read the first sector
		
		if(getleu32(&sector[0x100]) != magic) return false;	// verify magic
		
		printf(" NCSD image detected!\n Partitions:\n");
		
		// identify partitions
		for(int i=0; i<8; i++) {
			nand_partition_struct *partition = &dev_dnand.partitions[i];
			partition->offset = getleu32(&sector[0x120+(i<<3)]);
			partition->size = getleu32(&sector[0x120+(i<<3)+4]);
			partition->type = sector[0x110+i];
			switch(partition->type)
			{
				case 1:
					if(i==0) {
						partition->keyslot = 0x03;
						name = "twln";
					}
					else if(i==4) {
						partition->keyslot = 0x04;	// TODO: handle New3DS
						name = "ctrnand";
					}
					break;
				case 3:
					partition->keyslot = 0x06;
					if(i==2) name = "firm0";
					else if(i==3) name = "firm1";
					break;
				case 4:
					partition->keyslot = 0x07;
					name = "agbsave";
					break;
				default: // unused
					partition->keyslot = 0xFF;
					name = "other";
			}
			// print partition info
			if(partition->size)
				printf(" %s:\n  offset: 0x%X, size: %d MiB\n", name, partition->offset << 9, partition->size >> 11);
		}
		
		dev_dnand.dev.initialized = true;
	}
	return true;
}

nand_partition_struct *find_partition(u32 offset, u32 size)
{
	for(int i=0; i<8; i++) {
		nand_partition_struct *partition = &dev_dnand.partitions[i];
		if((partition->offset <= offset) && (partition->size >= size)
			&& (partition->offset + partition->size >= offset + size))
		return partition;
	}
	return NULL;
}

bool sdmmc_dnand_read(u32 offset, u32 size, void *buf)
{	
	if(!dev_dnand.dev.initialized)
		return false;
		
	nand_partition_struct *partition = find_partition(offset >> 9, size >> 9);
	if(!partition) return false;
	
	if(partition->keyslot == 0xFF) return false;	// unknown partition type
	
	AES_selectKeyslot(partition->keyslot);
	printf("dec read keyslot: %i\n", partition->keyslot);
	
	AES_setCtrIvNonce(&dev_dnand.aes_ctx, dev_dnand.cid_hash, AES_INPUT_LITTLE | AES_INPUT_NORMAL_ORDER | AES_MODE_CTR, offset);
	
	AES_setCryptParams(&dev_dnand.aes_ctx, AES_FLUSH_READ_FIFO | AES_FLUSH_WRITE_FIFO | AES_BIT12 | AES_BIT13 | AES_OUTPUT_BIG |
						AES_INPUT_BIG | AES_OUTPUT_NORMAL_ORDER | AES_INPUT_NORMAL_ORDER | AES_MODE_CTR);
	
	if(sdmmc_nand_readsectors(offset >> 9, size >> 9, buf))
		return false;
	printf("dec!!\n");
	AES_crypt(&dev_dnand.aes_ctx, buf, buf, size);
	return true;
}

bool sdmmc_dnand_write(u32 offset, u32 size, void *buf)
{
	if(!dev_dnand.dev.initialized)
		return false;
	
	//return !sdmmc_nand_writesectors(offset >> 9, size >> 9, buf);
	printf("ugh\n");
	return false;
}

bool sdmmc_dnand_close()
{
	return true;
}

bool sdmmc_dnand_is_active()
{
	return sdmmc_rnand_is_active();
}


// ------------------------------ wifi flash glue functions ------------------------------

bool wififlash_init()
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

bool wififlash_close()
{
	// nothing to do here..?
	return true;
}

bool wififlash_is_active()
{
	if(dev_wififlash.initialized) return true;
	return wififlash_init();
}
