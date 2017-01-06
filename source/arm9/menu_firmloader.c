#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "mem_map.h"
#include "arm9/console.h"
#include "arm9/dev.h"
#include "fatfs/ff.h"
#include "hid.h"
#include "util.h"
#include "arm9/main.h"
#include "arm9/timer.h"
#include "arm9/menu.h"
#include "arm9/firm.h"
#include "arm9/config.h"

static bool firmLoaded = 0;

static bool tryLoadFirmware(const char *filepath);
static u32 loadFirmSd(const char *filePath);

bool isFirmLoaded(void)
{
	return firmLoaded;
}

bool menuLaunchFirm(const char *filePath)
{
	u32 keys;
	
	if(!filePath)
		return false;
	
	firmLoaded = 0;
	
	clearConsoles();
	consoleSelect(&con_top);
	
	printf("FIRM Loader\n\n");

	menuUpdateGlobalState();
	
	if(!tryLoadFirmware(filePath))
	{
		printf("\n\x1B[31mBad firmware.\e[0m\n\n");
		printf("Press B to return.\n");
		
		do {
			hidScanInput();
			keys = hidKeysDown();
		}
		while(!(keys & KEY_B));
		
		goto fail;
	}
	
	firmLoaded = 1;
	
	printf("\n\x1B[32mFirmware verification SUCCESS!\e[0m\n\n");
	
	printf("Attempting to run firmware, press B to exit...\n");
	
	for(int i=0; i<0x800000; i++)
	{
		hidScanInput();
		keys = hidKeysDown();
		
		if(keys & KEY_B)
			goto fail;
	}
	
	menuSetReturnToState(MENU_STATE_EXIT);
	
	return true;

fail:
	
	printf("FIRM-Launch aborted.\n");
	
	TIMER_sleep(2000);
	
	firmLoaded = 0;
	
	menuSetReturnToState(STATE_PREVIOUS);
	
	return false;
}

// Does very basic checks whether the firmware actually exists.
bool statFirmware(const char *filePath)
{
	FILINFO fileStat;
	u32 fileSize;
	
	ensure_mounted(filePath);

	if(strncmp(filePath, "sdmc:", 5) == 0)
	{
		if(f_stat(filePath, &fileStat) != FR_OK)
			return false;
		
		fileSize = fileStat.fsize;
		
		if(fileSize == 0 || FIRM_MAX_SIZE > FIRM_MAX_SIZE)
			return false;
		
		return true;
	}
	
	// unknown mountpoint
	return false;
}

bool tryLoadFirmwareFromSettings(void)
{
	const char *path;
	int keyBootOption, keyPad;
	int i;
	u32 padValue, expectedPadValue;

	firmLoaded = 0;

	clearConsoles();
	consoleSelect(&con_top);

	printf("Loading FIRM from settings\n\n");

	keyBootOption = KBootOption1;
	keyPad = KBootOption1Buttons;

	for(i=0; i<3; i++, keyBootOption++, keyPad++)
	{
		path = (const char *)configGetData(keyBootOption);
		if(path)
		{
			printf("\nBoot Option #%i:\n%s\n", i + 1, path);
			
			// check pad value
			const u32 *temp;
			temp = (const u32 *)configGetData(keyPad);
			if(temp)
			{
				expectedPadValue = *temp;
				hidScanInput();
				padValue = HID_KEY_MASK_ALL & hidKeysHeld();
				if(padValue != expectedPadValue)
				{
					printf("Skipping, right buttons are not pressed.\n");
					printf("%" PRIX32 " %" PRIX32 "\n", padValue, expectedPadValue);
					continue;
				}
			}
			
			// check if fw still exists
			if(!statFirmware(path))
			{
				printf("Couldn't find firmware...\n");
				continue;
			}

			if(menuLaunchFirm(path))
				break;
			else
			{
				clearConsoles();
				consoleSelect(&con_top);
			}
		}

		// ... we failed, try next one
	}

	if(i >= 3)
		return false;

	return true;
}

bool menuTryLoadFirmwareFromSettings(void)
{
	const char *path;
	int keyBootOption, keyPad;
	int i;
	u32 padValue, expectedPadValue;

	clearConsoles();
	consoleSelect(&con_top);

	printf("Loading FIRM from settings\n\n");

	keyBootOption = KBootOption1;
	keyPad = KBootOption1Buttons;

	for(i=0; i<3; i++, keyBootOption++, keyPad++)
	{
		path = (const char *)configGetData(keyBootOption);
		if(path)
		{
			printf("\nBoot Option #%i:\n%s\n", i + 1, path);
			
			// check if fw still exists
			if(!statFirmware(path))
			{
				printf("Couldn't find firmware...\n");
				continue;
			}
			
			// check pad value
			const u32 *temp;
			temp = (const u32 *)configGetData(keyPad);
			if(temp)
			{
				expectedPadValue = *temp;
				hidScanInput();
				padValue = HID_KEY_MASK_ALL & hidKeysHeld();
				if(padValue != expectedPadValue)
				{
					printf("Skipping, right buttons are not pressed.\n");
					printf("%" PRIX32 " %" PRIX32 "\n", padValue, expectedPadValue);
					continue;
				}
			}

			if(menuLaunchFirm(path))
				break;
			else
			{
				clearConsoles();
				consoleSelect(&con_top);
			}
		}

		// ... we failed, try next one
	}

	if(i >= 3)
	{
		menuSetReturnToState(STATE_PREVIOUS);
		return false;
	}
	
	menuSetReturnToState(MENU_STATE_EXIT);

	return true;
}

static bool tryLoadFirmware(const char *filepath)
{
	u32 fw_size;

	if(!filepath)
		return false;

	printf("Loading firmware:\n%s\n\n", filepath);

	if(strncmp(filepath, "sdmc:", 5) == 0)
		fw_size = loadFirmSd(filepath);
	else
		return false;	// TODO: Support more devices

	if(fw_size == 0)
		return false;

	return firm_verify(fw_size);
}

static u32 loadFirmSd(const char *filePath)
{
	FIL file;
	u32 fileSize;
	UINT bytesRead = 0;
	FILINFO fileStat;

	if(f_stat(filePath, &fileStat) != FR_OK)
	{
		printf("Failed to get file status!\n");
		return 0;
	}

	fileSize = fileStat.fsize;

	if(f_open(&file, filePath, FA_READ) != FR_OK)
	{
		printf("Failed to open '%s'!\n", filePath);
		return 0;
	}

	if(fileSize == 0 || FIRM_MAX_SIZE > FIRM_MAX_SIZE)
	{
		f_close(&file);
		return 0;
	}

	if(f_read(&file, (u8*)FIRM_LOAD_ADDR, fileSize, &bytesRead) != FR_OK)
	{
		printf("Failed to read from file!\n");
		fileSize = 0;
	}

	if(bytesRead != fileSize)
		fileSize = 0;

	f_close(&file);

	return fileSize;
}
