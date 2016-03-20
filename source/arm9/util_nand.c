#include <stdio.h>
#include "types.h"
#include "mem_map.h"
#include "arm9/console.h"
#include "arm9/dev.h"
#include "arm9/fatfs/ff.h"
#include "arm9/firm.h"
#include "arm9/sdmmc.h"
#include "arm9/crypto.h"
#include "hid.h"
#include "arm9/ndma.h"

u64 getFreeSpace(const char *dev)
{
	DWORD freeClusters;
	FATFS *fs;

	if(f_getfree(dev, &freeClusters, &fs) != FR_OK)
	{
		printf("Failed to get free SD card space!\n");
		return 0;
	}
	return ((u64)(freeClusters * fs->csize)) * 512;
}

bool dumpNand(const char *filepath)
{
	u32 nandSectorCnt = getMMCDevice(0)->total_size;
	const u32 sectorBlkSize = 0x4000;
	u32 curSector = 0;
	FIL file;
	UINT bytesWritten;
	u32 blockSize;
	bool res = true;
	u8 *buf = (u8 *) FCRAM_BASE;

	if(getFreeSpace("sdmc") < (nandSectorCnt * 512))
	{
		printf("Not enough space on the SD card!\n");
		return false;
	}
	
	if(f_open(&file, filepath, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
	{
		printf("Failed to create %s!\n", filepath);
		return false;
	}

	for(u32 i = 0; i <= nandSectorCnt / sectorBlkSize; i++)
	{
		blockSize = ((nandSectorCnt - curSector < sectorBlkSize) ? nandSectorCnt - curSector : sectorBlkSize);

		if(blockSize > 0)
		{
			if(!dev_rawnand->read(curSector, blockSize, buf))
			{
				printf("\nFailed to read sector 0x%X!\n", (unsigned int)curSector);
				res = false;
				break;
			}
			if(f_write(&file, buf, blockSize * 512, &bytesWritten) != FR_OK)
			{
				printf("\nFailed to write to file!\n");
				res = false;
				break;
			}

			curSector += blockSize;
			printf("\r%d%% (Sector 0x%X/0x%X)", (int)(curSector * 100 / nandSectorCnt), (unsigned int)curSector, (unsigned int)nandSectorCnt);
		}
	}

	f_close(&file);
	return res;
}

bool restoreNand(const char *filepath)
{
	FIL file;
	UINT bytesRead;
	u32 blockSize;
	bool res = true;
	const u32 bufSize = 0x00800000;
	u64 offset = 0;
	u8 *buf = (u8 *) FCRAM_BASE;


	FILINFO fileStat;
	if(f_stat(filepath, &fileStat) != FR_OK)
	{
		printf("Failed to get file status!\n");
		return false;
	}
	if(fileStat.fsize > (getMMCDevice(0)->total_size * 512))
	{
		printf("NAND file is bigger than NAND!\n");
		return false;
	}

	if(f_open(&file, filepath, FA_READ) != FR_OK)
	{
		printf("Failed to open '%s'!\n", filepath);
		return false;
	}

	u64 size = fileStat.fsize;
	for(u32 i = 0; i <= size / bufSize; i++)
	{
		blockSize = ((size - offset < bufSize) ? size - offset : bufSize);

		if(blockSize > 0)
		{
			if(f_read(&file, buf, blockSize, &bytesRead) != FR_OK)
			{
				printf("\nFailed to read from file!\n");
				res = false;
				break;
			}
			if(!dev_rawnand->write((u32)(offset>>9), blockSize>>9, buf))
			{
				printf("\nFailed to write sector 0x%X!\n", (unsigned int)offset>>9);
				res = false;
				break;
			}

			offset += blockSize;
			printf("\r%d%% (Sector 0x%X/0x%X)", (int)(offset * 100 / size), (unsigned int)(offset>>9), (unsigned int)(size>>9));
		}
	}

	f_close(&file);
	return res;
}
