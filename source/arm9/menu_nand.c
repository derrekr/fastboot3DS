#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "mem_map.h"
#include "arm9/console.h"
#include "arm9/dev.h"
#include "fatfs/ff.h"
#include "hid.h"
#include "util.h"
#include "arm9/main.h"
#include "arm9/menu.h"
#include "arm9/timer.h"
#include "arm9/partitions.h"
#include "arm9/firmwriter.h"
#include "arm9/nandimage.h"
#include "arm9/fsutils.h"


bool menuDumpNand(const char *filePath)
{
	const u32 sectorCount = dev_rawnand->get_sector_count();
	const u32 sectorBlkSize = 0x400; // 512 KB in sectors
	FIL file;
	FRESULT fres;
	UINT bytesWritten;
	u64 bytesFree;

	u8 *buf = (u8*)malloc(sectorBlkSize<<9);
	if(!buf)
	{	// this should never happen.
		uiPrintIfVerbose("Out of memory!");
		return false;
	}

	uiClearConsoles();
	consoleSelect(&con_top);

	uiPrintCenteredInLine(1, "NAND Backup");
	
	uiPrintTextAt(0, 3, "Checking free space on SD card...\n");
	
	if(!fsGetFreeSpaceOnDrive("sdmc:", &bytesFree))
		uiPrintError("Failed to get free space!\n");
	
	if(bytesFree < sectorCount<<9)
	{
		uiPrintError("Not enough space on the SD card!\n");
		goto fail;
	}
	
	uiPrintTextAt(0, 4, "Creating image file...\n");

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
	
	uiPrintTextAt(0, 5, "Dumping NAND...\n");
	uiPrintTextAt(0, 22, "Press B to cancel.");

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
			uiPrintTextAt(0, 22, "... canceled.     "); 
			f_close(&file);
			goto fail;
		}

		curSector += curSectorBlkSize;

		// print current progress information
		uiPrintTextAt(1, 20, "\r%"PRId32"/%"PRId32" MB (Sector 0x%"PRIX32"/0x%"PRIX32")", curSector>>11, sectorCount>>11, 
				curSector, sectorCount);
		
		uiPrintProgressBar(10, 80, 380, 20, curSector, sectorCount);
		
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
	
	uiDialog("Finished!\nPress any key to return.", NULL, HID_KEY_MASK_ALL, 1, 0, 0, true);	
	
	menuSetReturnToState(STATE_PREVIOUS);
	
	return true;

fail:
	free(buf);
	
	uiPrintTextAt(0, 24, "Press any key to return.");
	menuWaitForAnyPadkey();
	
	menuSetReturnToState(STATE_PREVIOUS);
	
	return false;
}

bool menuRestoreNand(const char *filePath)
{
	const u32 sectorBlkSize = 0x400; // 512 KB in sectors
	FIL file;
	UINT bytesRead;
	
	u8 *buf = (u8*)malloc(sectorBlkSize<<9);
	if(!buf)
	{	// this should never happen.
		uiPrintIfVerbose("Out of memory!");
		return false;
	}

	uiClearConsoles();
	consoleSelect(&con_top);
	
	uiPrintCenteredInLine(1, "NAND Restore");
	
	uiPrintTextAt(0, 3, "Checking backup file...\n");

	FILINFO fileStat;
	if(f_stat(filePath, &fileStat) != FR_OK)
	{
		uiPrintError("Failed to get file status!\n");
		goto fail;
	}
	if(fileStat.fsize > dev_rawnand->get_sector_count()<<9)
	{
		uiPrintError("NAND file is bigger than NAND!\n");
		goto fail;
	}
	if(fileStat.fsize < 0x200)
	{
		uiPrintError("NAND file is too small!\n");
		goto fail;
	}

	if(f_open(&file, filePath, FA_READ) != FR_OK)
	{
		uiPrintError("Failed to open '%s'!\n", filePath);
		f_close(&file);
		goto fail;
	}
	
	if(!isNandImageCompatible(&file))
	{
		uiPrintError("NAND file is not compatible with this console!\n");
		f_close(&file);
		goto fail;
	}
	
	uiPrintTextAt(0, 4, "Unmounting NAND fs...\n");

	fsUnmountNandFilesystems();

	uiPrintTextAt(0, 5, "Restoring...\n");
	uiPrintTextAt(0, 22, "Press B to cancel."); 
	
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
			uiPrintError("\nFailed to read from file!\n");
			f_close(&file);
			goto fail;
		}
		if(!dev_rawnand->write_sector(curSector, curSectorBlkSize, buf))
		{
			uiPrintError("\nFailed to write sector 0x%"PRIX32"!\n", curSector);
			f_close(&file);
			goto fail;
		}
		
		hidScanInput();
		if(hidKeysDown() & KEY_B)
		{
			uiPrintTextAt(0, 22, "... canceled.     "); 
			f_close(&file);
			goto fail;
		}

		curSector += curSectorBlkSize;
		
		uiPrintTextAt(1, 20, "\r%"PRId32"/%"PRId32" MB (Sector 0x%"PRIX32"/0x%"PRIX32")", curSector>>11, sectorCount>>11, 
				curSector, sectorCount);
		
		uiPrintProgressBar(10, 80, 380, 20, curSector, sectorCount);

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
	fsRemountNandFilesystems();
	
	uiPrintTextAt(0, 24, "Finished! Press any key to return.");
	menuWaitForAnyPadkey();
	
	menuSetReturnToState(STATE_PREVIOUS);
	
	return true;

fail:
	free(buf);
	fsRemountNandFilesystems();
	
	uiPrintTextAt(0, 24, "Press any key to return.");
	menuWaitForAnyPadkey();
	
	menuSetReturnToState(STATE_PREVIOUS);
	
	return false;
}

bool menuFlashFirmware(const char *filepath)
{
	const char *partName = "firm1";	// TODO let the user decide
	FILINFO fileStat;
	size_t fwSize;
	size_t index;
	size_t sector;
	size_t numSectors;

	uiClearConsoles();
	consoleSelect(&con_top);
	
	uiPrintCenteredInLine(1, "Flash firmware");
	
	uiPrintTextAt(0, 3, "Verifying file...\n");
	
	if(f_stat(filepath, &fileStat) != FR_OK)
	{
		uiPrintError("Could not retrieve file status!\n");
		goto fail;
	}
	
	if(!tryLoadFirmware(filepath, false, false) || !firm_size(&fwSize))
	{
		uiPrintError("Firmware is invalid or corrupted!\n");
		goto fail;
	}

	if(!partitionGetIndex(partName, &index))
	{
		uiPrintError("Could not find partition %s!", partName);
		goto fail;
	}
	
	partitionGetSectorOffset(index, &sector);
	
	if(!uiDialogYesNo(1, "Proceed", "Cancel", "Flash firmware to %s?", partName))
	{
		goto fail;
	}

	uiPrintTextAt(0, 4, "Writing...\n");
	
	if(!firmwriterInit(sector, fwSize / 0x200, false))
	{
		uiPrintError("Failed to start flashing process!");
		goto fail;
	}
	
	for(size_t i=0; i<fwSize / 0x200; )
	{
		if(!firmwriterIsDone())
		{
			/* Write one block in each iteration and update ui */
			numSectors = firmwriterWriteBlock();
			if(!numSectors)
			{
				uiPrintError("Writing block failed!");
				goto fail;
			}
		}
		else
		{
			uiPrintTextAt(0, 21, "Finalizing...");
			
			numSectors = firmwriterFinish();
			if(!numSectors)
			{
				uiPrintError("Failed writing block!");
				goto fail;
			}
		}
		
		i+= numSectors;
		
		uiPrintTextAt(1, 20, "\r%"PRId32"/%"PRId32, i, fwSize / 0x200);

		uiPrintProgressBar(10, 80, 380, 20, i, fwSize / 0x200);
	}

	uiPrintTextAt(0, 24, "Success! Press any key to return.");
	menuWaitForAnyPadkey();

	menuSetReturnToState(STATE_PREVIOUS);
	
	return true;
	
fail:
	
	uiPrintTextAt(0, 24, "Press any key to return...");
	menuWaitForAnyPadkey();
	
	menuSetReturnToState(STATE_PREVIOUS);
	
	return false;
}
