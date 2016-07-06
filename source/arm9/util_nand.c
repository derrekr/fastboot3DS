#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "mem_map.h"
#include "arm9/console.h"
#include "arm9/dev.h"
#include "arm9/fatfs/ff.h"
#include "hid.h"
#include "util.h"
#include "arm9/util_nand.h"



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
	const u32 sectorCount = dev_rawnand->get_sector_count();
	const u32 sectorBlkSize = 0x400; // 512 KB in sectors
	FIL file;
	FRESULT fres;
	UINT bytesWritten;


	consoleSelect(&con_bottom);
	printf("\n\t\tPress B to cancel");
	consoleSelect(&con_top);

	u8 *buf = (u8*)malloc(sectorBlkSize<<9);
	if(!buf)
	{
		printf("Not enough memory!\n");
		return false;
	}

	if(getFreeSpace("sdmc:") < sectorCount<<9)
	{
		printf("Not enough space on the SD card!\n");
		goto fail;
	}

	if((fres = f_open(&file, filePath, FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK)
	{
		printf("Failed to create '%s'! Error: %X\n", filePath, fres);
		f_close(&file);
		goto fail;
	}

	u32 curSector = 0;
	u32 curSectorBlkSize;
	while(curSector < sectorCount)
	{
		if(curSector + sectorBlkSize < sectorCount) curSectorBlkSize = sectorBlkSize;
		else curSectorBlkSize = sectorCount - curSector;

		if(!dev_rawnand->read_sector(curSector, curSectorBlkSize, buf))
		{
			printf("\nFailed to read sector 0x%"PRIx32"!\n", curSector);
			f_close(&file);
			goto fail;
		}
		if((f_write(&file, buf, curSectorBlkSize<<9, &bytesWritten) != FR_OK) || (bytesWritten != curSectorBlkSize<<9))
		{
			printf("\nFailed to write to file!\n");
			f_close(&file);
			goto fail;
		}

		hidScanInput();
		if(hidKeysDown() & KEY_B)
		{
			printf("\n...canceled!\n");
			f_close(&file);
			goto fail;
		}

		curSector += curSectorBlkSize;
		printf("\r%"PRId32"/%"PRId32" MB (Sector 0x%"PRIX32"/0x%"PRIX32")", curSector>>11, sectorCount>>11, 
				curSector, sectorCount);
	}

	f_close(&file);
	free(buf);
	return true;

fail:
	free(buf);
	sleep_wait(0x8000000);
	return false;
}

bool restoreNand(const char *filePath)
{
	const u32 sectorBlkSize = 0x400; // 512 KB in sectors
	FIL file;
	UINT bytesRead;


	consoleSelect(&con_bottom);
	printf("\n\t\tPress B to cancel");
	consoleSelect(&con_top);

	u8 *buf = (u8*)malloc(sectorBlkSize<<9);
	if(!buf)
	{
		printf("Not enough memory!\n");
		goto fail;
	}

	FILINFO fileStat;
	if(f_stat(filePath, &fileStat) != FR_OK)
	{
		printf("Failed to get file status!\n");
		goto fail;
	}
	if(fileStat.fsize > dev_rawnand->get_sector_count()<<9)
	{
		printf("NAND file is bigger than NAND!\n");
		goto fail;
	}
	if(fileStat.fsize < 0x200)
	{
		printf("NAND file is too small!\n");
		goto fail;
	}

	if(f_open(&file, filePath, FA_READ) != FR_OK)
	{
		printf("Failed to open '%s'!\n", filePath);
		f_close(&file);
		goto fail;
	}


	const u32 sectorCount = fileStat.fsize>>9;
	u32 curSector = 0;
	u32 curSectorBlkSize;
	while(curSector < sectorCount)
	{
		if(curSector + sectorBlkSize < sectorCount) curSectorBlkSize = sectorBlkSize;
		else curSectorBlkSize = sectorCount - curSector;

		if(f_read(&file, buf, curSectorBlkSize<<9, &bytesRead) != FR_OK || bytesRead != curSectorBlkSize<<9)
		{
			printf("\nFailed to read from file!\n");
			f_close(&file);
			goto fail;
		}
		if(!dev_rawnand->write_sector(curSector, curSectorBlkSize, buf))
		{
			printf("\nFailed to write sector 0x%"PRIX32"!\n", curSector);
			f_close(&file);
			goto fail;
		}

		curSector += curSectorBlkSize;
		printf("\r%"PRId32"/%"PRId32" MB (Sector 0x%"PRIX32"/0x%"PRIX32")", curSector>>11, sectorCount>>11, 
				curSector, sectorCount);
	}

	f_close(&file);
	free(buf);
	return true;

fail:
	free(buf);
	sleep_wait(0x8000000);
	return false;
}
