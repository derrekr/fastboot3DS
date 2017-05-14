#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "mem_map.h"
#include "arm9/console.h"
#include "arm9/dev.h"
#include "fatfs/ff.h"
#include "hid.h"
#include "util.h"
#include "arm9/menu.h"
#include "arm9/timer.h"
#include "arm9/util_nand.h"


static u64 getFreeSpace(const char *drive)
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

bool menuDumpNand(const char *filePath)
{
	const u32 sectorCount = dev_rawnand->get_sector_count();
	const u32 sectorBlkSize = 0x400; // 512 KB in sectors
	FIL file;
	FRESULT fres;
	UINT bytesWritten;

	uiClearConsoles();
	consoleSelect(&con_top);

	u8 *buf = (u8*)malloc(sectorBlkSize<<9);
	if(!buf)
		panic();	// this should never happen.

	if(getFreeSpace("sdmc:") < sectorCount<<9)
	{
		uiPrintError("Not enough space on the SD card!\n");
		goto fail;
	}

	if((fres = f_open(&file, filePath, FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK)
	{
		uiPrintError("Failed to create '%s'! Error: %X\n", filePath, fres);
		f_close(&file);
		goto fail;
	}
	
	// Allocate space to make sure the nand image is a contiguous file
	if((fres = f_expand(&file, sectorCount<<9, 0)) != FR_OK)
	{
		uiPrintError("Failed to expand file! Error: %X\n", fres);
		f_close(&file);
		goto fail;
	}

	/* Main loop */

	u32 curSector = 0;
	u32 curSectorBlkSize;
	while(curSector < sectorCount)
	{
		if(curSector + sectorBlkSize < sectorCount) curSectorBlkSize = sectorBlkSize;
		else curSectorBlkSize = sectorCount - curSector;

		if(!dev_rawnand->read_sector(curSector, curSectorBlkSize, buf))
		{
			uiPrintError("\nFailed to read sector 0x%"PRIx32"!\n", curSector);
			f_close(&file);
			goto fail;
		}
		if((f_write(&file, buf, curSectorBlkSize<<9, &bytesWritten) != FR_OK) || (bytesWritten != curSectorBlkSize<<9))
		{
			uiPrintError("\nFailed to write to file!\n");
			f_close(&file);
			goto fail;
		}

		hidScanInput();
		if(hidKeysDown() & KEY_B)
		{
			uiPrintInfo("\n...canceled!\n");
			f_close(&file);
			goto fail;
		}

		curSector += curSectorBlkSize;

		// print current progress information
		uiPrintInfo("\r%"PRId32"/%"PRId32" MB (Sector 0x%"PRIX32"/0x%"PRIX32")", curSector>>11, sectorCount>>11, 
				curSector, sectorCount);

		
		uiPrintProgressBar(10, 10, 200, 100 * curSector/sectorCount);
		
		switch(menuUpdateGlobalState())
		{
			case MENU_EVENT_HOME_PRESSED:
			case MENU_EVENT_POWER_PRESSED:
				f_close(&file);
			case MENU_EVENT_SD_CARD_REMOVED:
				menuActState();
				goto fail;
			default:
				break;
		}

		menuActState();
	}

	f_close(&file);
	free(buf);
	
	menuSetReturnToState(STATE_PREVIOUS);
	
	return true;

fail:
	free(buf);
	
	uiPrintInfo("Press any key to return...");
	menuWaitForAnyPadkey();
	
	menuSetReturnToState(STATE_PREVIOUS);
	
	return false;
}

bool menuRestoreNand(const char *filePath)
{
	const u32 sectorBlkSize = 0x400; // 512 KB in sectors
	FIL file;
	UINT bytesRead;

	uiClearConsoles();

	consoleSelect(&con_bottom);
	printf("\n\t\tPress B to cancel");
	consoleSelect(&con_top);

	u8 *buf = (u8*)malloc(sectorBlkSize<<9);
	if(!buf)
		panic();	// this should never happen.

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


	unmount_nand_fs();

	/* Main loop */

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

		switch(menuUpdateGlobalState())
		{
			case MENU_EVENT_HOME_PRESSED:
			case MENU_EVENT_POWER_PRESSED:
				f_close(&file);
			case MENU_EVENT_SD_CARD_REMOVED:
				menuActState();
				goto fail;
			default:
				break;
		}

		menuActState();
	}

	f_close(&file);
	free(buf);
	remount_nand_fs();
	
	menuSetReturnToState(STATE_PREVIOUS);
	
	return true;

fail:
	free(buf);
	remount_nand_fs();
	
	printf("Press any key to return...");
	menuWaitForAnyPadkey();
	
	menuSetReturnToState(STATE_PREVIOUS);
	
	return false;
}
