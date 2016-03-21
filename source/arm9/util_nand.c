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

bool dumpNand(const char *filePath)
{
	u32 nandSectorCnt = getMMCDevice(0)->total_size;
	u32 sectorBlkSize = 0x4000, curSector = 0;
	u8 *buf = (u8*)FCRAM_BASE;
	FIL file;
	UINT bytesWritten;
	u32 blockSize;
	bool res = true;


	if(getFreeSpace("sdmc:") < nandSectorCnt<<9)
	{
		printf("Not enough space on the SD card!\n");
		return false;
	}
	if(f_open(&file, filePath, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
	{
		printf("Failed to create '%s'!\n", filePath);
		return false;
	}

	while(curSector < nandSectorCnt)
	{
		blockSize = ((nandSectorCnt - curSector < sectorBlkSize) ? nandSectorCnt - curSector : sectorBlkSize);

		if(!dev_rawnand->read(curSector<<9, blockSize<<9, buf))
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

bool restoreNand(const char *filePath)
{
	FIL file;
	UINT bytesRead;
	u32 blockSize;
	bool res = true;
	u32 offset = 0;
	u32 bufSize = 0x800000;
	u8 *buf = (u8*)FCRAM_BASE;


	FILINFO fileStat;
	if(f_stat(filePath, &fileStat) != FR_OK)
	{
		printf("Failed to get file status!\n");
		return false;
	}
	if(fileStat.fsize > (getMMCDevice(0)->total_size * 512))
	{
		printf("NAND file is bigger than NAND!\n");
		return false;
	}

	if(f_open(&file, filePath, FA_READ) != FR_OK)
	{
		printf("Failed to open '%s'!\n", filePath);
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
		if(!dev_rawnand->write(offset, blockSize, buf))
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
