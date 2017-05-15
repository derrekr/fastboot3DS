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

#define MIN_UPDATE_SIZE	0x1000

static const char *updateFilePath = "sdmc:\\fastboot3ds.bin";
static const char *installPath = "firm0"

bool menuUpdateLoader()
{
	u8 *updateBuffer = (u8 *) FIRM_LOAD_ADDR;
	FILINFO fileStat;
	size_t fwSize;
	size_t sector;

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

	// XXX
	fwSize = calcFirmwareSize();

	if(fwSize < MIN_UPDATE_SIZE || fwSize % 0x200)	// this would be really odd.
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

	/* NOTE: We assume sighax is installed on firm0 */

	sector = partitionGetSector(installPath);

	uiPrintTextAt(0, 4, "Updating...\n");
	
	firmwriterInit(sector, fwSize / 0x200, true);
	
	for(size_t i=0; i<fwSize / 0x200; i++)
	{
			if(!firmwriterIsDone())
			{
				/* Write one sector in each iteration and update ui */
				if(!firmwriterWriteBlock())
				{
					uiPrintError("Failed writing block!");
					goto fail;
				}
			}
			else
			{
				if(!firmwriterFinish())
				{
					uiPrintError("Failed writing block!");
					goto fail;
				}

			}
        
			uiPrintTextAt(1, 20, "\r%"PRId32"/%"PRId32", i, fwSize / 0x200);

			uiPrintProgressBar(10, 80, 380, 20, i, fwSize / 0x200);

			
 
	}

	/* We should be done now... */

	


	uiPrintInfo("Press any key to reboot...");
	menuWaitForAnyPadkey();

	panic();
	
fail:
	
	uiPrintInfo("Press any key to return...");
	menuWaitForAnyPadkey();
	
	menuSetReturnToState(STATE_PREVIOUS);
	
	return false;
}
