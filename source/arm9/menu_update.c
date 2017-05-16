#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "mem_map.h"
#include "arm9/console.h"
#include "arm9/dev.h"
#include "arm9/partitions.h"
#include "fatfs/ff.h"
#include "hid.h"
#include "util.h"
#include "version.h"
#include "arm9/firmwriter.h"
#include "arm9/main.h"
#include "arm9/menu.h"
#include "arm9/timer.h"

#define MIN_UPDATE_SIZE	0x1000

static const char *updateFilePath = "sdmc:\\fastboot3ds.bin";
static const char *installPath = "firm0";

bool menuUpdateLoader()
{
	u8 *updateBuffer = (u8 *) FIRM_LOAD_ADDR;
	FILINFO fileStat;
	size_t fwSize;
	size_t index;
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
		uiPrintError("Update file is corrupted!\n");
		goto fail;
	}

	if(!firm_size(&fwSize) || fwSize < MIN_UPDATE_SIZE || fwSize % 0x200)
	{
		// this would be really odd.
		uiPrintError("Invalid update file!\n");
		goto fail;
	}
	
	// verify fastboot magic
	if(memcmp(updateBuffer+0x208, "FASTBOOT 3DS   ", 0x10) != 0)
	{
		uiPrintError("Not an update file!\n");
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
	
	if(!uiDialogYesNo("Update", "Cancel", "Update to v%" PRIu16 ".%" PRIu16,
			updateVersion>>16, updateVersion & 0xFFFFu))
	{
		goto fail;
	}

	/* NOTE: We assume sighax is installed on firm0 */

	if(!partitionGetIndex(installPath, &index))
	{
		uiPrintError("Could not find partition %s!", installPath);
		goto fail;
	}
	
	partitionGetSectorOffset(index, &sector);
	// printf("writing to offset 0x%x aka 0x%x\n", sector, sector<<9);

	uiPrintTextAt(0, 4, "Updating...\n");
	
	firmwriterInit(sector, fwSize / 0x200, true);
	
	size_t numSectors;
	
	for(size_t i=1; i<fwSize / 0x200 + 1; )
	{
		if(!firmwriterIsDone())
		{
			/* Write one block in each iteration and update ui */
			numSectors = firmwriterWriteBlock();
			if(!numSectors)
			{
				uiPrintError("Failed writing block!");
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
     
		uiPrintTextAt(1, 20, "\r%"PRId32"/%"PRId32, i, fwSize / 0x200);

		uiPrintProgressBar(10, 80, 380, 20, i, fwSize / 0x200);
		
		i+= numSectors;
	}

	/* We should be done now... */

	uiPrintTextAt(0, 24, "Press any key to reboot...");
	menuWaitForAnyPadkey();

	reboot_safe();
	
	return true;
	
fail:
	
	uiPrintTextAt(0, 24, "Press any key to return...");
	menuWaitForAnyPadkey();
	
	menuSetReturnToState(STATE_PREVIOUS);
	
	return false;
}
