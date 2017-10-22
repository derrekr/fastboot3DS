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

#include <stdlib.h>
#include "types.h"
#include "arm11/hardware/hardware.h"
#include "arm11/hardware/interrupt.h"
#include "arm11/hardware/hid.h"
#include "arm11/power.h"
#include "arm11/hardware/i2c.h"
#include "arm11/menu/menu.h"
#include "arm11/menu/menu_fb3ds.h"
#include "arm11/menu/menu_util.h"
#include "hardware/pxi.h"
#include "hardware/gfx.h"
#include "arm11/firm.h"
#include "arm11/console.h"
#include "arm11/debug.h"
#include "arm11/fmt.h"
#include "fsutils.h"
#include "arm11/config.h"


volatile bool g_continueBootloader = true;
volatile bool g_startFirmLaunch = false;



int main(void)
{
	bool show_menu = false;
	bool gfx_initialized = false;
	char* err_string = NULL;
	
	
	// init hardware / filesystem / load config
	hardwareInit();
	fsMountSdmc();
	fsMountNandFilesystems();
	loadConfigFile();
	
	
	// check keys held at boot
	hidScanInput();
	u32 kHeld = hidKeysHeld();
	
	// boot slot keycombo held?
	if (kHeld & 0xfff)
	{
		err_string = (char*) malloc(512);
		char* err_ptr = err_string;
		if(!err_string) panicMsg("Out of memory");
		
		char combo_str[0x80];
		keysToString(kHeld & 0xfff, combo_str);
		err_ptr += ee_sprintf(err_ptr, "Keys held on boot: %s\n", combo_str);
		
		for(u32 i = 0; (i < 3) && !g_startFirmLaunch; i++)
		{
			if(!configDataExist(KBootOption1 + i) ||
				!configDataExist(KBootOption1Buttons + i))
				continue;
			
			u32 combo = *(u32*) configGetData(KBootOption1Buttons + i);
			if((kHeld & 0xfff) == combo)
			{
				char* path = (char*) configGetData(KBootOption1 + i);
				err_ptr += ee_sprintf(err_ptr, "Keys match boot slot #%lu.\nBoot path is %s\n", (i+1), path);
				g_startFirmLaunch = (loadVerifyFirm(path, false) >= 0);
				err_ptr += ee_sprintf(err_ptr, "Load slot #%lu %s.\n", (i+1), g_startFirmLaunch ? "success" : "failed");
				break;
			}
		}
		
		if (!g_startFirmLaunch)
		{
			err_ptr += ee_sprintf(err_ptr, "Firmware not loaded!\nKeys may not match a boot slot.\n");
		}
		else
		{
			free(err_string);
			err_string = NULL;
		}
	}
	
	
	// get bootmode from config
	u32 bootmode = BootModeNormal;
	if(configDataExist(KBootMode))
		bootmode = *(u32*) configGetData(KBootMode);
	
	// show menu if either bootmode is normal or HOME is held
	// ... or not if we already got a firmware waiting in line
	show_menu = show_menu ||
		(!g_startFirmLaunch && ((bootmode == BootModeNormal) || hidIsHomeButtonHeldRaw()));

	
	// main loop
	while(!g_startFirmLaunch)
	{
		PrintConsole term_con;
	
		// init screens (if we'll need them below)
		if((show_menu || err_string) && !gfx_initialized)
		{
			GFX_init();
			// init and select terminal console
			consoleInit(SCREEN_TOP, &term_con, true);
			consoleSelect(&term_con);
			gfx_initialized = true;
		}
		
		// if there is an error, output it to screen
		if (err_string)
		{
			ee_printf("%s\nPress B or HOME to enter menu.\n", err_string);
			updateScreens();
			outputEndWait();
			
			show_menu = true;
			free(err_string);
			err_string = NULL;
			
			if(hidGetPowerButton(false)) break;
		}
			
		// menu specific code
		if(show_menu)
		{
			// init menu console
			PrintConsole menu_con;
			consoleInit(SCREEN_SUB, &menu_con, false);

			// run menu
			g_continueBootloader = false;
			menuProcess(&menu_con, &term_con, menu_fb3ds);
			
			// clear consoles and screen
			consoleSelect(&menu_con);
			consoleClear();
			consoleSelect(&term_con);
			consoleClear();
			updateScreens();
			
			// write config (if something changed)
			if (configHasChanged()) writeConfigFile();
			
			// check POWER button
			if(hidGetPowerButton(false)) break;
		}
		
		// search for a bootable firmware (all slots)
		if (g_continueBootloader && !g_startFirmLaunch)
		{
			err_string = (char*) malloc(512);
			char* err_ptr = err_string;
			if(!err_string) panicMsg("Out of memory");
		
			err_ptr += ee_sprintf(err_ptr, "Continuing bootloader...\n");
			for(u32 i = 0; (i < 3) && !g_startFirmLaunch; i++)
			{
				if(!configDataExist(KBootOption1 + i))
					continue;

				char* path = (char*) configGetData(KBootOption1 + i);
				err_ptr += ee_sprintf(err_ptr, "Trying boot slot #%lu.\nBoot path is %s\n", (i+1), path);
				g_startFirmLaunch = (loadVerifyFirm(path, false) >= 0);
				err_ptr += ee_sprintf(err_ptr, "Load slot #%lu %s.\n", (i+1), g_startFirmLaunch ? "success" : "failed");
				break;
			}
			
			if (!g_startFirmLaunch)
			{
				err_ptr += ee_sprintf(err_ptr, "Firmware not loaded!\nAre boot slots properly set up?\n");
			}
			else
			{
				free(err_string);
				err_string = NULL;
			}
		}
	}
	
	
	// deinit GFX if it was initialized
	if(gfx_initialized) GFX_deinit();
	
	// deinit filesystem
	fsUnmountAll();
	
	// free memory (should already be free at this time)
	if(err_string) free(err_string);
	
	// power off
	if(hidGetPowerButton(true)) power_off();

	// firm launch
	if(g_startFirmLaunch) firmLaunch();
	
	
	// reaching here was never intended....
	return 0;
}
