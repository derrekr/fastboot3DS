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
#include "arm11/hardware/hid.h"
#include "arm11/menu/bootslot_store.h"
#include "arm11/menu/menu.h"
#include "arm11/menu/menu_fb3ds.h"
#include "arm11/menu/menu_util.h"
#include "arm11/menu/splash.h"
#include "arm11/bootenv.h"
#include "arm11/console.h"
#include "arm11/config.h"
#include "arm11/debug.h"
#include "arm11/firm.h"
#include "arm11/fmt.h"
#include "arm11/power.h"
#include "hardware/gfx.h"
#include "banner_spla.h"
#include "fsutils.h"


volatile bool g_continueBootloader = true;
volatile u32 g_startFirmLaunch = 0;



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
		
		bool has_error = false;
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
				has_error = !(g_startFirmLaunch = (loadVerifyFirm(path, false) >= 0) ? (i+1) : 0);
				err_ptr += ee_sprintf(err_ptr, "Load slot #%lu %s.\n", (i+1), g_startFirmLaunch ? "success" : "failed");
				break;
			}
		}
		
		if (!has_error)
		{
			free(err_string);
			err_string = NULL;
		}
	}
	
	
	// get bootmode from config and previous bootslot from I2C
	u32 prevBootSlot = getBootEnv() ? readStoredBootslot() : 0;
	u32 bootmode = BootModeNormal;
	if(configDataExist(KBootMode))
		bootmode = *(u32*) configGetData(KBootMode);
	
	// show menu if bootmode is normal, no firmware is waiting in line and not rebooting to slot
	// ... or always if the HOME button is pressed
	show_menu = show_menu ||
		(!g_startFirmLaunch && !prevBootSlot && (bootmode == BootModeNormal)) ||
		hidIsHomeButtonHeldRaw();
	
	// show splash if (bootmode != BootModeSilent)
	if(show_menu || (bootmode != BootModeQuiet))
	{
		GFX_init();
		gfx_initialized = true;
		
		if (drawSplashscreen(banner_spla, -1, -1))
		{
			for (u32 i = 0; i < 64; i++) // VBlank * 64
				GFX_waitForEvent(GFX_EVENT_PDC0, true); 
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
			consoleInit(SCREEN_TOP, &term_con, true); // <-- maybe not do this everytime? (!!!)
			consoleSelect(&term_con);
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
			
			// previous bootslot not known -> try all autoboot slots
			if (!prevBootSlot)
			{
				err_ptr += ee_sprintf(err_ptr, "Continuing bootloader...\n");
				for (u32 i = 0; (i < 3) && !g_startFirmLaunch; i++)
				{
					// FIRM not set for slot or slot not set to autoboot
					if (!configDataExist(KBootOption1 + i) || configDataExist(KBootOption1Buttons + i))
						continue;

					char* path = (char*) configGetData(KBootOption1 + i);
					err_ptr += ee_sprintf(err_ptr, "Trying boot slot #%lu.\nBoot path is %s\n", (i+1), path);
					g_startFirmLaunch = (loadVerifyFirm(path, false) >= 0) ? (i+1) : 0;
					err_ptr += ee_sprintf(err_ptr, "Load slot #%lu %s.\n", (i+1), g_startFirmLaunch ? "success" : "failed");
					break;
				}
			}
			// boot env handling (only on reboots) for firm1:
			else if (prevBootSlot == FIRM1_BOOT_SLOT)
			{
				err_ptr += ee_sprintf(err_ptr, "Rebooting to firm1:...\n");
				g_startFirmLaunch = (loadVerifyFirm("firm1:", false) >= 0) ? prevBootSlot : 0;
				err_ptr += ee_sprintf(err_ptr, "Load firm1: %s.\n", g_startFirmLaunch ? "success" : "failed");
			}
			// boot env handling (only on reboots) -> try previous slot
			else
			{
				err_ptr += ee_sprintf(err_ptr, "Rebooting to boot slot #%lu...\n", prevBootSlot);
		
				if(!configDataExist(KBootOption1 + (prevBootSlot-1)))
				{
					err_ptr += ee_sprintf(err_ptr, "Could not find entry in config!\n");
				}
				else
				{
					char* path = (char*) configGetData(KBootOption1 + (prevBootSlot-1));
					err_ptr += ee_sprintf(err_ptr, "Boot path is %s\n", path);
					g_startFirmLaunch = (loadVerifyFirm(path, false) >= 0) ? prevBootSlot : 0;
					err_ptr += ee_sprintf(err_ptr, "Load slot #%lu %s.\n", prevBootSlot, g_startFirmLaunch ? "success" : "failed");
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
		if((g_startFirmLaunch <= 3) || (g_startFirmLaunch == 0xFE))
			storeBootslot(g_startFirmLaunch);
		else storeBootslot(0);
		firmLaunch();
	}
	
	
	// reaching here was never intended....
	return 0;
}
