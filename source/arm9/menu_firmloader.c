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
#include "arm9/timer.h"
#include "arm9/menu.h"
#include "arm9/firm.h"

static bool tryLoadFirmware(const char *filepath);
static u32 loadFirmSd(const char *filePath);

bool menuLaunchFirm(const char *filePath)
{
	u32 keys;
	
	consoleSelect(&con_bottom);
	consoleClear();
	consoleSelect(&con_top);
	consoleClear();
	
	printf("FIRM Loader\n\n");

	menuUpdateGlobalState();
	
	if(!tryLoadFirmware(filePath))
	{
		printf("\x1B[31mBad firmware.\e[0m\n");
		goto fail;
	}
	
	printf("\x1B[32mFirmware verification SUCCESS!\e[0m\n\n");
	
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
	
	menuSetReturnToState(STATE_PREVIOUS);
	
	return false;
}

static bool tryLoadFirmware(const char *filepath)
{
	u32 fw_size;

	if(!filepath)
		return false;

	printf("Loading firmware:\n%s\n", filepath);

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

	if(fileSize > FIRM_MAX_SIZE)
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
