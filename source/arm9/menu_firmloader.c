#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "mem_map.h"
#include "arm9/console.h"
#include "arm9/dev.h"
#include "fatfs/ff.h"
#include "arm9/hid.h"
#include "util.h"
#include "pxi.h"
#include "arm9/main.h"
#include "arm9/timer.h"
#include "arm9/menu.h"
#include "arm9/firm.h"
#include "arm9/config.h"
#include "arm9/partitions.h"
#include "arm9/fsutils.h"

static int firmLoaded = 0;

bool isFirmLoaded(void)
{
	return firmLoaded != 0;
}

bool menuLaunchFirm(const char *filePath, bool quick)
{
	if(!filePath)
		return false;
	
	firmLoaded = 0;
	
	uiClearConsoles();
	consoleSelect(&con_top);
	
	uiPrintCenteredInLine(1, "FIRM Loader");

	menuUpdateGlobalState();
	
	if(!tryLoadFirmware(filePath, false, true))
	{
		if(!quick)
		{
			uiPrintError("Bad firmware.\n");
			uiPrintInfo("Press any key...\n");
			
			menuWaitForAnyPadkey();
		}
		goto fail;
	}
	
	firmLoaded = 1;
	
	uiPrint("Firmware verification SUCCESS!", 32, true);
	
	menuSetReturnToState(MENU_STATE_EXIT);
	
	return true;

fail:
	
	uiPrintCenteredInLine(25, "FIRM-Launch aborted.");
	
	TIMER_sleep(500);
	
	firmLoaded = 0;
	
	menuSetReturnToState(STATE_PREVIOUS);
	
	return false;
}

// Does very basic checks whether the firmware actually exists.
static bool statFirmware(const char *filePath)
{
	FILINFO fileStat;
	// u32 fileSize;
	
	fsEnsureMounted(filePath);

	if(strncmp(filePath, "sdmc:", 5) == 0)
	{
		if(f_stat(filePath, &fileStat) != FR_OK)
			return false;
		
		/*
		fileSize = fileStat.fsize;
		
		if(fileSize == 0 || fileSize > FIRM_MAX_SIZE)
			return false;
		*/
		
		return true;
	}
	else if(strncmp(filePath, "firm1:", 5) == 0)
		return true;	// this is always available
	
	// unknown mountpoint
	return false;
}

static bool checkForHIDAbort()
{
	// give it 10 ms to check whether home button was pressed
	return uiCheckHomePressed(10);
}

bool tryLoadFirmwareFromSettings(bool fromMenu)
{
	const char *path;
	int keyBootOption, bootMode, keyPad;
	int i;
	u32 padValue, expectedPadValue;

	if(fromMenu)
		uiClearConsoles();
	
	consoleSelect(&con_top);
	
	firmLoaded = 0;
	
	bootInfo.numBootOptionsAttempted = 0;
	bootInfo.bootOptionResults[0]  = BO_NOT_ATTEMPTED;
	bootInfo.bootOptionResults[1]  = BO_NOT_ATTEMPTED;
	bootInfo.bootOptionResults[2]  = BO_NOT_ATTEMPTED;

	if(fromMenu)
		uiPrintCenteredInLine(1, "Loading FIRM from settings\n");
	
	// No Boot Option set up?
	if(!configDataExist(KBootOption1) &&
		!configDataExist(KBootOption2) &&
		!configDataExist(KBootOption3))
	{
		if(fromMenu)
		{
			menuPrintPrompt("No Boot-Option set up yet.\nGo to 'Options' to choose a file.\n");
			menuWaitForAnyPadkey();
			menuSetReturnToState(STATE_PREVIOUS);
		}
		return false;
	}
	
	const u32 *temp = configGetData(KBootMode);
	bootMode = temp? *temp : BootModeNormal;

	keyBootOption = KBootOption1;
	keyPad = KBootOption1Buttons;

	for(i=0; i<3; i++, keyBootOption++, keyPad++)
	{
		if(checkForHIDAbort())
			return false;
		
		bootInfo.numBootOptionsAttempted ++;
		
		path = (const char *)configGetData(keyBootOption);
		if(path)
		{
			if(fromMenu)
				uiPrintInfo("Boot Option #%i:\n%s\n", i + 1, path);
			
			// check if fw still exists
			if(!statFirmware(path))
			{
				if(fromMenu)
					uiPrintInfo("Couldn't find firmware...\n");
				bootInfo.bootOptionResults[i] = BO_NOT_FOUND;
				goto try_next;
			}
			
			// check pad value
			const u32 *dataPtr;
			dataPtr = (const u32 *)configGetData(keyPad);
			if(dataPtr)
			{
				expectedPadValue = *dataPtr;
				
				if(expectedPadValue != 0)
				{	
					hidScanInput();
					padValue = HID_KEY_MASK_ALL & hidKeysHeld();
					if(padValue != expectedPadValue)
					{
						if(fromMenu)
						{
							uiPrintInfo("Skipping, right buttons are not pressed.\n");
							uiPrintInfo("%" PRIX32 " %" PRIX32 "\n", padValue, expectedPadValue);
						}
						bootInfo.bootOptionResults[i] = BO_SKIPPED;
						goto try_next;
					}
				}
			}

			if(fromMenu)
			{
				if(menuLaunchFirm(path, false))
					break;
				else
				{
					uiClearConsoles();
					consoleSelect(&con_top);
				}
			}
			else
			{
				if(tryLoadFirmware(path, false, false))
					break;
				else
				{
					if(bootMode != BootModeQuiet)
						uiPrintBootWarning();
				}
			}
			
			bootInfo.bootOptionResults[i] = BO_FAILED;
		}
		else
			continue;
		
try_next:
		
		if(fromMenu)
			TIMER_sleep(300);

		// ... we failed, try next one
	}

	if(i >= 3)
	{
		if(fromMenu)
			menuSetReturnToState(STATE_PREVIOUS);
		else if(bootMode != BootModeQuiet)
			uiPrintBootFailure();
		return false;
	}
	
	if(checkForHIDAbort())
		return false;
	
	firmLoaded = 1;
	
	if(fromMenu)
		menuSetReturnToState(MENU_STATE_EXIT);

	return true;
}

static u32 loadFirmSd(const char *filePath)
{
	FIL file;
	u32 fileSize;
	UINT bytesRead = 0;
	FILINFO fileStat;

	if(f_stat(filePath, &fileStat) != FR_OK)
	{
		uiPrintInfo("Failed to get file status!\n");
		return 0;
	}

	fileSize = fileStat.fsize;

	if(f_open(&file, filePath, FA_READ) != FR_OK)
	{
		uiPrintInfo("Failed to open '%s'!\n", filePath);
		return 0;
	}

	if(fileSize == 0 || fileSize > FIRM_MAX_SIZE)
	{
		f_close(&file);
		return 0;
	}

	if(f_read(&file, (u8*)FIRM_LOAD_ADDR, fileSize, &bytesRead) != FR_OK)
	{
		uiPrintInfo("Failed to read from file!\n");
		fileSize = 0;
	}

	if(bytesRead != fileSize)
		fileSize = 0;

	f_close(&file);

	return fileSize;
}

static u32 loadFirmNandPartition(const char *filePath)
{
	char partName[11];
	void *firmBuf = (void *) FIRM_LOAD_ADDR;
	size_t index;
	size_t sector;
	size_t firmSize;
	
	sscanf(filePath, "%10s:", partName);
	
	if(!partitionGetIndex(partName, &index))
		return 0;
	
	partitionGetSectorOffset(index, &sector);

	/* get header to figure out the actual firm size */
	if(!dev_decnand->read_sector(sector, 1, firmBuf))
		return 0;
	
	if(!firm_size(&firmSize))
		return 0;

	/* read the rest */

	if(!dev_decnand->read_sector(sector + 1, firmSize - 1, firmBuf + 0x200))
		return 0;

	return firmSize;
}

bool tryLoadFirmware(const char *filepath, bool skipHashCheck, bool printInfo)
{
	u32 fw_size;

	if(!filepath)
		return false;
	
	if(!statFirmware(filepath))
		return false;

	// printf("Loading firmware:\n%s\n\n", filepath);

	/* SD card */
	if(strncmp(filepath, "sdmc:", 5) == 0)
		fw_size = loadFirmSd(filepath);
	/* NAND */
	else if(strncmp(filepath, "firm1:", 5) == 0)
		fw_size = loadFirmNandPartition(filepath);
	else
		return false;	// TODO: Support more devices

	if(fw_size == 0)
		return false;

	if(!firm_verify(fw_size, skipHashCheck, printInfo)) return false;

	((const char**)(ITCM_KERNEL_MIRROR + 0x7470))[0] = ((const char*)(ITCM_KERNEL_MIRROR + 0x7474));
	strncpy_s((void*)(ITCM_KERNEL_MIRROR + 0x7474), filepath, 256, 256);

	return true;
}
