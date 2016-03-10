#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "types.h"
#include "arm9/console.h"
#include "mem_map.h"
#include "arm9/fatfs/ff.h"
#include "arm9/firm.h"
#include "arm9/sdmmc.h"
#include "arm9/crypto.h"
#include "arm9/utils.h"



bool loadFirmNand(void)
{
	u32 hash[8];
	AES_ctx aesCtx;
	u32 size = 0;
	u8 *buf = (u8*)FIRM_LOAD_ADDR;


	// Hash the NAND CID the bootrom left for us in ITCM.
	sha((void*)0x01FFCD84, 16, hash, SHA_INPUT_BIG | SHA_MODE_256, SHA_OUTPUT_LITTLE);

	// Setup AES engine.
	AES_selectKeyslot(6); // firmX:/ keyslot
	AES_setCtrIvNonce(&aesCtx, hash, AES_INPUT_LITTLE | AES_INPUT_NORMAL_ORDER | AES_MODE_CTR, 0x0B530000);
	AES_setCryptParams(&aesCtx, AES_FLUSH_READ_FIFO | AES_FLUSH_WRITE_FIFO | AES_BIT12 | AES_BIT13 | AES_OUTPUT_BIG |
						AES_INPUT_BIG | AES_OUTPUT_NORMAL_ORDER | AES_INPUT_NORMAL_ORDER | AES_MODE_CTR);

	// Read FIRM header from NAND and decrypt it.
	if(sdmmc_nand_readsectors(0x0B530000>>9, 1, buf))
	{
		printf("Failed to read FIRM header!\n");
		return false;
	}
	AES_crypt(&aesCtx, buf, buf, 512);
	buf += 512;

	// Calculate remaining size.
	firm_header *header = (firm_header*)FIRM_LOAD_ADDR;
	for(int i = 0; i < 4; i++) size += header->section[i].size;

	// Read remaining data.
	if(sdmmc_nand_readsectors(0x0B530200>>9, size>>9, buf))
	{
		printf("Failed to read FIRM sections!\n");
		return false;
	}

	// Decrypt remaining data of FIRM.
	AES_crypt(&aesCtx, buf, buf, size);

	return true;
}

bool loadFile(const char *filePath, void *address, u32 size, u32 *bytesRead)
{
	FIL file;
	u32 tmp;
	bool res = true;


	if(bytesRead == NULL) bytesRead = &tmp;
	if(f_open(&file, filePath, FA_READ) != FR_OK)
	{
		printf("Failed to open '%s'!\n", filePath);
		return false;
	}
	if(f_read(&file, address, size, (UINT*)bytesRead) != FR_OK)
	{
		printf("Failed to read from file!\n");
		res = false;
	}
	f_close(&file);

	return res;
}

bool makeWriteFile(const char *filePath, void *address, u32 size, u32 *bytesWritten)
{
	FIL file;
	u32 tmp;
	bool res = true;


	if(bytesWritten == NULL) bytesWritten = &tmp;
	if(f_open(&file, filePath, FA_CREATE_ALWAYS|FA_WRITE) != FR_OK)
	{
		printf("Failed to create '%s'!\n", filePath);
		return false;
	}
	if(f_write(&file, address, size, (UINT*)bytesWritten) != FR_OK)
	{
		printf("Failed to write to file!\n");
		res = false;
	}
	f_close(&file);

	return res;
}

bool updateNandLoader(const char *filePath)
{
	u32 hash[8];
	AES_ctx aesCtx;
	u32 size, cmpVal = 0xAAAAAAAA;
	u8 *buf = (u8*)0x20800000;
	firm_header *header = (firm_header*)0x20800000;


	// Hash the NAND CID the bootrom left for us in ITCM.
	sha((void*)0x01FFCD84, 16, hash, SHA_INPUT_BIG | SHA_MODE_256, SHA_OUTPUT_LITTLE);

	// Setup AES engine.
	AES_selectKeyslot(6); // firmX:/ keyslot
	AES_setCtrIvNonce(&aesCtx, hash, AES_INPUT_LITTLE | AES_INPUT_NORMAL_ORDER | AES_MODE_CTR, 0x0B130000);
	AES_setCryptParams(&aesCtx, AES_FLUSH_READ_FIFO | AES_FLUSH_WRITE_FIFO | AES_BIT12 | AES_BIT13 | AES_OUTPUT_BIG |
						AES_INPUT_BIG | AES_OUTPUT_NORMAL_ORDER | AES_INPUT_NORMAL_ORDER | AES_MODE_CTR);

	// Read FIRM from file.
	if(!loadFile(filePath, buf, FIRM_MAX_SIZE, &size)) return false;

	// Check for hax.
	if(memcmp(header->signature, &cmpVal, 4) == 0)
	{
		printf("Invalid FIRM!\n");
		return false;
	}

	// Delete update file.
	if(f_unlink(filePath) != FR_OK)
	{
		printf("Failed to delete '%s'!\n", filePath);
		return false;
	}

	// Encrypt FIRM.
	AES_crypt(&aesCtx, buf, buf, size);

	// Write FIRM to firm0:/.
	if(sdmmc_nand_writesectors(0x0B130000>>9, size>>9, buf))
	{
		printf("Failed to write sector 0xB130000!\n");
		return false;
	}

	printf("Loader successfully updated.\n");
	return true;
}

u64 getFreeSpace(const char *drive)
{
	FATFS *fs;
	DWORD freeClusters;

	if(f_getfree(drive, &freeClusters, &fs) != FR_OK)
	{
		printf("Failed to get free space for '%s'!\n", drive);
		return 0;
	}
	return ((u64)(freeClusters * fs->csize)) * 512;
}

bool dumpNand(void)
{
	u32 nandSectorCnt = getMMCDevice(0)->total_size;
	u32 sectorBlkSize = 0x4000, curSector = 0;
	u8 *buf = (u8*)0x20800000;
	FIL file;
	UINT bytesWritten;
	u32 blockSize;
	bool res = true;


	if(getFreeSpace("sdmc:") < nandSectorCnt<<9)
	{
		printf("Not enough space on the SD card!\n");
		return false;
	}
	if(f_open(&file, "sdmc:/NAND.bin", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
	{
		printf("Failed to create '%s'!\n", "sdmc:/NAND.bin");
		return false;
	}

	while(curSector < nandSectorCnt)
	{
		blockSize = ((nandSectorCnt - curSector < sectorBlkSize) ? nandSectorCnt - curSector : sectorBlkSize);

		if(sdmmc_nand_readsectors(curSector, blockSize, buf))
		{
			printf("\nFailed to read sector 0x%X!\n", (unsigned int)curSector);
			res = false;
			break;
		}
		if(f_write(&file, buf, blockSize<<9, &bytesWritten) != FR_OK || bytesWritten>>9 != blockSize)
		{
			printf("\nFailed to write to file!\n");
			res = false;
			break;
		}

		curSector += blockSize;
		printf("\r%u%% (Sector 0x%X/0x%X)", (unsigned int)(curSector * 100 / nandSectorCnt), (unsigned int)curSector, (unsigned int)nandSectorCnt);
	}

	f_close(&file);
	return res;
}

bool restoreNand(void)
{
	FIL file;
	UINT bytesRead;
	u32 blockSize;
	bool res = true;
	u32 offset = 0;
	u32 bufSize = 0x800000;
	u8 *buf = (u8*)0x20800000;


	FILINFO fileStat;
	if(f_stat("sdmc:/NAND.bin", &fileStat) != FR_OK)
	{
		printf("Failed to get file status!\n");
		return false;
	}
	if(fileStat.fsize > (getMMCDevice(0)->total_size * 512))
	{
		printf("NAND file is bigger than NAND!\n");
		return false;
	}

	if(f_open(&file, "sdmc:/NAND.bin", FA_READ) != FR_OK)
	{
		printf("Failed to open '%s'!\n", "sdmc:/NAND.bin");
		return false;
	}

	u32 size = fileStat.fsize;
	while(offset < size)
	{
		blockSize = ((size - offset < bufSize) ? size - offset : bufSize);

		if(f_read(&file, buf, blockSize, &bytesRead) != FR_OK || bytesRead != blockSize)
		{
			printf("\nFailed to read from file!\n");
			res = false;
			break;
		}
		if(sdmmc_nand_writesectors(offset>>9, blockSize>>9, buf))
		{
			printf("\nFailed to write sector 0x%X!\n", (unsigned int)offset>>9);
			res = false;
			break;
		}

		offset += blockSize;
		printf("\r%d%% (Sector 0x%X/0x%X)", (unsigned int)((u64)offset * 100 / size), (unsigned int)offset>>9, (unsigned int)size>>9);
	}

	f_close(&file);
	return res;
}

void initGfx(void)
{
	static bool isInitialized = false;

	if(!isInitialized)
	{
		isInitialized = true;
		consoleInit(1, NULL);
		*((vu32*)CORE_SYNC_ID) = 1; // Tell ARM11 to turn on gfx.
		while(*((vu32*)CORE_SYNC_ID) == 1);
	}
}
