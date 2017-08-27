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
#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "mem_map.h"
#include "util.h"
#include "hardware/pxi.h"
#include "arm9/hardware/hid.h"
#include "arm9/hardware/hardware.h"
#include "arm9/debug.h"
#include "arm9/hardware/interrupt.h"
#include "fatfs/ff.h"
#include "arm9/dev.h"
#include "arm9/fsutils.h"
#include "arm9/firm.h"
#include "arm9/config.h"
#include "arm9/hardware/timer.h"
#include "arm9/start.h"
#include "hardware/cache.h"



int main(void)
{
	//int mode = BootModeNormal;

	hardwareInit();

	debugHashCodeRoData();
	
	/*uiInit();
	
	checkSetVerboseMode();
	
	consoleSelect(&con_bottom);
	
	uiPrintIfVerbose("\x1B[32mGood morning\nHello !\e[0m\n\n");*/
	
	//PXI_sendWord(PXI_CMD_ALLOW_POWER_OFF);
	
	/*uiPrintIfVerbose("Detecting unit...\n");

	unit_detect();

	initWifiFlash();

	uiPrintIfVerbose("Filesystem init...\n");*/
	
	/* Try to read the settings file ASAP. */
	/*if(mount_fs() > 0) // We got at least 1 drive mounted
	{
		uiPrintIfVerbose("Loading settings...\n");
	
		if(loadSettings(&mode))
		{
			uiPrintIfVerbose("Trying to launch FIRM from settings...\n");
		
			switch(mode)
			{
				case BootModeQuick:
					screen_init();
					uiDrawSplashScreen();*/
					/* fallthrough */
				/*case BootModeQuiet:
					tryLoadFirmwareFromSettings(false);
					if(isFirmLoaded())
						goto finish_firmlaunch;*/
					/* else fallthrough */
				/*case BootModeNormal:
					screen_init();
					break;
				default:
					panic();
			}
		}
		else screen_init();
	}
	
	if(mode != BootModeQuick)
		uiDrawSplashScreen();

	uiPrintIfVerbose("Detecting boot environment...\n");

	boot_env_detect();

	uiPrintIfVerbose("Detecting firmware...\n");

	fw_detect();

	uiPrintIfVerbose("Entering menu...\n");*/

	/* In verbose mode wait before we clear all debug messages */
	/*if(uiGetVerboseMode())
		TIMER_sleep(600);

	consoleClear();

	enter_menu(MENU_STATE_MAIN);

	
finish_firmlaunch:

	uiPrintIfVerbose("Unmounting FS...\n");

	fsUnmountAll();
	
	uiPrintIfVerbose("Closing devices...\n");

	devs_close();

	hardwareDeinit();

	// TODO: Proper argc/v passing needs to be implemented.
	firm_launch(1, (const char**)(ITCM_KERNEL_MIRROR + 0x7470));*/

	return 0;
}
