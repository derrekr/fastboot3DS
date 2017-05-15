#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "mem_map.h"
#include "arm9/console.h"
#include "arm9/dev.h"
#include "fatfs/ff.h"
#include "hid.h"
#include "util.h"
#include "version.h"
#include "arm9/main.h"
#include "arm9/menu.h"
#include "arm9/timer.h"

static const char *updateFilePath = "sdmc:\\fastboot3ds.bin";

bool menuUpdateLoader()
{
	u8 *updateBuffer = (u8 *) FIRM_LOAD_ADDR;
	FILINFO fileStat;

	uiClearConsoles();
	consoleSelect(&con_top);
	
	uiPrintCenteredInLine(1, "fastboot Updater");
	
	uiPrintTextAt(0, 3, "Checking update file...\n");
	
	if(f_stat(updateFilePath, &fileStat) != FR_OK)
	{
		uiPrintError("Could not find fastboot3ds.bin!\n");
		goto fail;
	}
	
	// update file is just a firmware
	if(!tryLoadFirmware(updateFilePath, false, false))
	{
		uiPrintError("Invalid update file!\n");
		goto fail;
	}
	
	// verify fastboot magic
	if(memcmp(updateBuffer+0x208, "FASTBOOT 3DS   ", 0x10) != 0)
	{
		uiPrintError("Invalid update file!\n");
		goto fail;
	}
	
	/* check version */
	u32 updateVersion = *(u32 *)(updateBuffer + 0x218);
	
	if(updateVersion < BOOTLOADER_VERSION)
	{
		uiPrintError("Update version is below current version!\n");
		goto fail;
	}
	
	if(updateVersion == BOOTLOADER_VERSION)
	{
		uiPrintWarning("You are on this version already.\n");
		goto fail;
	}
	
	if(!uiDialogYesNo("Do you want to update to version XXX?", "Update", "Cancel"))
	{
		goto fail;
	}

	uiPrintTextAt(0, 4, "Updating...\n");
	
	
fail:
	
	uiPrintInfo("Press any key to return...");
	menuWaitForAnyPadkey();
	
	menuSetReturnToState(STATE_PREVIOUS);
	
	return false;
}
