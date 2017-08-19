/*
 *   This file is part of fastboot 3DS
 *   Copyright (C) 2017 derrek, profi200
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "mem_map.h"
#include "arm9/console.h"
#include "arm9/dev.h"
#include "arm9/partitions.h"
#include "fatfs/ff.h"
#include "arm9/hardware/hid.h"
#include "util.h"
#include "arm9/firmwriter.h"
#include "arm9/main.h"
#include "arm9/gui/menu.h"
#include "arm9/hardware/timer.h"
#include "arm9/hardware/crypto.h"
#include "fastboot3DS_pubkey_bin.h"

#define MIN_UPDATE_SIZE	0x1000

static const char *updateFilePath = "sdmc:/fastboot3DS.firm";
static const char *installPath = "firm0";

bool menuUpdateLoader()
{
	u32 *updateBuffer = (u32 *) FIRM_LOAD_ADDR;
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

	// Verify signature
	if(!RSA_setKey2048(3, fastboot3DS_pubkey_bin, 0x01000100) ||
	   !RSA_verify2048(updateBuffer + 0x40, updateBuffer, 0x100))
	{
		uiPrintError("Update file signature verification failed!\n");
		goto fail;
	}

	if(!firm_size(&fwSize) || fwSize < MIN_UPDATE_SIZE || fwSize % 0x200)
	{
		// this would be really odd.
		uiPrintError("Invalid update file!\n");
		goto fail;
	}
	
	// verify fastboot magic
	if(memcmp((void*)updateBuffer+0x200, "FASTBOOT 3DS   ", 0x10) != 0)
	{
		uiPrintError("Not an update file!\n");
		goto fail;
	}
	
	/* check version */
	u32 updateVersion = *(u32 *)((void*)updateBuffer + 0x210);
	
	if(updateVersion < ((u32)VERS_MAJOR<<16 | VERS_MINOR))
	{
		uiPrintError("Update version is below current version!\n");
		goto fail;
	}
	
	if(updateVersion == ((u32)VERS_MAJOR<<16 | VERS_MINOR))
	{
		uiPrintWarning("You are on this version already.\n");
		goto fail;
	}
	
	if(!uiDialogYesNo(1, "Update", "Cancel", "Update to v%" PRIu16 ".%" PRIu16,
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
	// ee_printf("writing to offset 0x%x aka 0x%x\n", sector, sector<<9);

	uiPrintTextAt(0, 9, "Updating...\n");
	
	if(!firmwriterInit(sector, fwSize / 0x200, true))
	{
		uiPrintError("Failed to start update process!");
		goto fail;
	}
	
	size_t numSectors;
	
	for(size_t i=1; i<fwSize / 0x200 + 1; )
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
