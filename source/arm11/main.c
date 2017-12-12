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
#include "arm11/hardware/hid.h"
#include "arm11/menu/bootslot.h"
#include "arm11/menu/menu.h"
#include "arm11/menu/menu_fb3ds.h"
#include "arm11/menu/menu_util.h"
#include "arm11/menu/splash.h"
#include "arm11/console.h"
#include "arm11/config.h"
#include "arm11/debug.h"
#include "arm11/firm.h"
#include "arm11/fmt.h"
#include "arm11/power.h"
#include "hardware/gfx.h"
#include "banner_spla.h"
#include "fsutils.h"


volatile bool g_startFirmLaunch = false;



int main(void)
{
	bool show_menu = false;
	bool gfx_initialized = false;
	
	s32 firm_err = 0; // local result of loadVerifyFirm()
	char* err_string = NULL;
	
	
	// filesystem / load config
	fsMountSdmc();
	fsMountNandFilesystems();
	loadConfigFile();
	
	
	// get bootmode from config, previous bootslot from I2C
	u32 nextBootSlot = readStoredBootslot();
	u32 bootmode = BootModeNormal;
	if(configDataExist(KBootMode))
		bootmode = *(u32*) configGetData(KBootMode);
	
	// show menu if bootmode is normal or HOME button is pressed
	show_menu = (!nextBootSlot && (bootmode == BootModeNormal)) || hidIsHomeButtonHeldRaw();
	
	// show splash if (bootmode != BootModeQuiet)
	bool splash_wait = false;
	if(show_menu || (bootmode != BootModeQuiet))
	{
		if (!gfx_initialized) GFX_init();
		gfx_initialized = true;
		splash_wait = drawSplashscreen(banner_spla, -1, -1);
	}
	
	
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
		
		for(u32 i = 0; (i < N_BOOTSLOTS) && !g_startFirmLaunch; i++)
		{
			if(!configDataExist(KBootOption1 + i) ||
				!configDataExist(KBootOption1Buttons + i))
				continue;
			
			u32 combo = *(u32*) configGetData(KBootOption1Buttons + i);
			if((kHeld & 0xfff) == combo)
			{
				char* path = (char*) configGetData(KBootOption1 + i);
				err_ptr += ee_sprintf(err_ptr, "Keys match boot slot #%lu.\nBoot path is %s\n", (i+1), path);
				
				firm_err = loadVerifyFirm(path, false);
				if (firm_err >= 0)
				{
					g_startFirmLaunch = true;
					storeBootslot(i + 1);
				}
				
				err_ptr += ee_sprintf(err_ptr, "Load slot #%lu %s.\n", (i+1), g_startFirmLaunch ? "success" : "failed");
				break;
			}
		}
		
		if (firm_err >= 0)
		{
			free(err_string);
			err_string = NULL;
		}
	}
	
	
	if (splash_wait)
	{
		for (u32 i = 0; i < 64; i++) // VBlank * 64
		{
			GFX_waitForEvent(GFX_EVENT_PDC0, true); 
			hidScanInput();
			if (hidKeysDown() & KEY_HOME)
			{
				show_menu = true;
				g_startFirmLaunch = false;
			}
		}
	}

	
	// main loop
	PrintConsole term_con;
	while(!g_startFirmLaunch)
	{
		// init screens / console (if we'll need them below)
		if(show_menu || err_string)
		{
			if (!gfx_initialized) GFX_init();
			gfx_initialized = true;
			// init and select terminal console
			consoleInit(SCREEN_TOP, &term_con, true);
			consoleSetWindow(&term_con, 1, 1, 64, 22);
			consoleSelect(&term_con);
			drawTopBorder();
		}
		
		// if there is an error, output it to screen
		if(err_string)
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
			menuProcess(&menu_con, &term_con, menu_fb3ds);
			
			// clear consoles and screen
			consoleSelect(&menu_con);
			consoleClear();
			consoleSelect(&term_con);
			consoleClear();
			clearTop();
			updateScreens();
			
			// write config (if something changed)
			if (configHasChanged()) writeConfigFile();
			
			// check POWER button
			if(hidGetPowerButton(false)) break;
		}
		
		// search for a bootable firmware (all slots)
		if (!g_startFirmLaunch)
		{
			err_string = (char*) malloc(512);
			char* err_ptr = err_string;
			if(!err_string) panicMsg("Out of memory");
			
			// previous bootslot not known -> try all autoboot slots
			if (!nextBootSlot)
			{
				err_ptr += ee_sprintf(err_ptr, "Continuing bootloader...\n");
				for (u32 i = 0; (i < N_BOOTSLOTS) && !g_startFirmLaunch; i++)
				{
					// FIRM not set for slot or slot not set to autoboot
					if (!configDataExist(KBootOption1 + i) || configDataExist(KBootOption1Buttons + i))
						continue;

					char* path = (char*) configGetData(KBootOption1 + i);
					err_ptr += ee_sprintf(err_ptr, "Trying boot slot #%lu.\nBoot path is %s\n", (i+1), path);
					
					firm_err = loadVerifyFirm(path, false);
					if (firm_err >= 0)
					{
						g_startFirmLaunch = true;
						storeBootslot(i + 1);
					}
					
					err_ptr += ee_sprintf(err_ptr, "Load slot #%lu %s.\n", (i+1), g_startFirmLaunch ? "success" : "failed");
					break;
				}
			}
			// boot env handling (only on reboots) -> try previous slot
			else
			{
				err_ptr += ee_sprintf(err_ptr, "Rebooting to boot slot #%lu...\n", nextBootSlot);
		
				if(!configDataExist(KBootOption1 + (nextBootSlot-1)))
				{
					err_ptr += ee_sprintf(err_ptr, "Could not find entry in config!\n");
				}
				else
				{
					char* path = (char*) configGetData(KBootOption1 + (nextBootSlot-1));
					err_ptr += ee_sprintf(err_ptr, "Boot path is %s\n", path);
					
					firm_err = loadVerifyFirm(path, false);
					g_startFirmLaunch = (firm_err >= 0);
					// no need to store the bootslot here as it stays as is
					
					err_ptr += ee_sprintf(err_ptr, "Load slot #%lu %s.\n", nextBootSlot, g_startFirmLaunch ? "success" : "failed");
				}
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
	if(hidGetPowerButton(true))
	{
		storeBootslot(0);
		power_off();
	}

	// firm launch
	if(g_startFirmLaunch)
	{
		// make sure GFX is in the right state
		if(!gfx_initialized && (firm_err == 1))
		{
			GFX_init();
			GFX_deinit();
		}
		
		// launch firm
		firmLaunch();
	}
	
	
	// reaching here was never intended....
	panicMsg("Reached code end!");
	return 0;
}
