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
#include "arm11/menu/menu_func.h" // the bootrom dumper is in there
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
#include "firmwriter.h"
#include "hardware/gfx.h"
#include "banner_spla.h"
#include "fsutils.h"

extern const bool __superhaxEnabled;



int main(void)
{
	bool startFirmLaunch = false;
	bool show_menu = false;
	bool dump_bootroms = __superhaxEnabled;
	bool gfx_initialized = false;
	u32 menu_ret = MENU_OK;
	
	s32 firm_err = 0; // local result of loadVerifyFirm()
	char* err_string = NULL;
	
	
	// filesystem / load config
	fsMountSdmc();
	fsMountNandFilesystems();
	loadConfigFile();


	// bootrom dumper?
	// skip bootslot / FCRAM / reboot handling in that case
	if(dump_bootroms)
	{
		// workaround when HOME button is held
		if (hidIsHomeButtonHeldRaw())
		{
			menu_ret = MENU_RET_REBOOT;
			if (toggleSuperhax(false) == 0)
			{
				goto menu_end;
			}
			else // this really should never happen
			{
				dump_bootroms = false;
				show_menu = true;
			}
		}
		goto menu_start;
	}
	
	
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
		if (!gfx_initialized) GFX_init(true);
		gfx_initialized = true;
		if(configDataExist(KSplashScreen))
		{
			char* folder = (char*) configGetData(KSplashScreen);
			splash_wait = drawCustomSplash(folder); // || drawSplashscreen(banner_spla, -1, -1, SCREEN_TOP);
			if (!splash_wait)
			{
				err_string = (char*) malloc(512);
				if(!err_string) panicMsg("Out of memory");
				ee_sprintf(err_string, "Custom splash screen error:\n%s\n", folder);
			}
		}
		else
		{
			splash_wait = drawSplashscreen(banner_spla, -1, -1, SCREEN_TOP);
		}
		updateScreens();
	}
	
	
	// check keys held at boot
	hidScanInput();
	u32 kHeld = hidKeysHeld();
	
	// boot slot keycombo held?
	if (!err_string && (kHeld & 0xfff))
	{
		err_string = (char*) malloc(512);
		char* err_ptr = err_string;
		if(!err_string) panicMsg("Out of memory");
		
		char combo_str[0x80];
		keysToString(kHeld & 0xfff, combo_str);
		err_ptr += ee_sprintf(err_ptr, "Keys held on boot: %s\n", combo_str);
		
		for(u32 i = 0; (i < N_BOOTSLOTS) && !startFirmLaunch; i++)
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
					startFirmLaunch = true;
					storeBootslot(i + 1);
				}
				
				err_ptr += ee_sprintf(err_ptr, "Load slot #%lu %s.\n", (i+1), startFirmLaunch ? "success" : "failed");
				break;
			}
		}
		
		if (firm_err >= 0)
		{
			free(err_string);
			err_string = NULL;
		}
	}
	
	
	// firm from FCRAM handling
	if (!startFirmLaunch && configRamFirmBootEnabled() &&
		(getBootEnv() == BOOTENV_NATIVE_FIRM))
	{
		if (loadVerifyFirm("ram", false) == 0)
			startFirmLaunch = true;
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
				startFirmLaunch = false;
			}
		}
	}

	
	menu_start: ;

	// main loop
	PrintConsole term_con;
	while(!startFirmLaunch)
	{
		// init screens / console (if we'll need them below)
		if(show_menu || err_string || dump_bootroms)
		{
			if (!gfx_initialized) GFX_init(true);
			gfx_initialized = true;
			// init and select terminal console
			consoleInit(SCREEN_TOP, &term_con, true);
			consoleSetWindow(&term_con, 1, 1, 64, 22);
			consoleSelect(&term_con);
			clearScreens();
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

		// bootrom dumper handling
		if(dump_bootroms)
		{
			menu_ret = menuDumpBootrom(&term_con, NULL, 0);
			if ((menu_ret == MENU_RET_POWEROFF) || (menu_ret == MENU_RET_REBOOT))
				break; // success
			else
				show_menu = true; // backup solution
		}
			
		// menu specific code
		if(show_menu)
		{
			// init menu console
			PrintConsole menu_con;
			consoleInit(SCREEN_SUB, &menu_con, false);

			// run menu
			menu_ret = menuProcess(&menu_con, &term_con, menu_fb3ds);
			
			// clear screens
			clearScreens();
			updateScreens();
			
			// write config (if something changed)
			if (configHasChanged()) writeConfigFile();
			
			// check POWER button
			if(hidGetPowerButton(false)) break;
			
			// check menu return value
			if (menu_ret == MENU_RET_FIRMLOADED)
			{
				startFirmLaunch = true;
				firm_err = 0;
			}
			else if (menu_ret == MENU_RET_FIRMLOADED_SI)
			{
				startFirmLaunch = true;
				firm_err = 1;
			}
			else if ((menu_ret == MENU_RET_POWEROFF) || (menu_ret == MENU_RET_REBOOT))
			{
				break;
			}
		}
		
		// search for a bootable firmware (all slots)
		if (!startFirmLaunch)
		{
			err_string = (char*) malloc(512);
			char* err_ptr = err_string;
			if(!err_string) panicMsg("Out of memory");
			
			// previous bootslot not known -> try all autoboot slots
			if (!nextBootSlot)
			{
				err_ptr += ee_sprintf(err_ptr, "Continuing bootloader...\n");
				for (u32 i = 0; (i < N_BOOTSLOTS) && !startFirmLaunch; i++)
				{
					// FIRM not set for slot or slot not set to autoboot
					if (!configDataExist(KBootOption1 + i) || configDataExist(KBootOption1Buttons + i))
						continue;

					char* path = (char*) configGetData(KBootOption1 + i);
					err_ptr += ee_sprintf(err_ptr, "Trying boot slot #%lu.\nBoot path is %s\n", (i+1), path);
					
					firm_err = loadVerifyFirm(path, false);
					if (firm_err >= 0)
					{
						startFirmLaunch = true;
						storeBootslot(i + 1);
					}
					
					err_ptr += ee_sprintf(err_ptr, "Load slot #%lu %s.\n", (i+1), startFirmLaunch ? "success" : "failed");
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
					startFirmLaunch = (firm_err >= 0);
					// no need to store the bootslot here as it stays as is
					
					err_ptr += ee_sprintf(err_ptr, "Load slot #%lu %s.\n", nextBootSlot, startFirmLaunch ? "success" : "failed");
				}
			}
			
			if (!startFirmLaunch)
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
	

	menu_end:
	
	// deinit GFX if it was initialized
	if(gfx_initialized) GFX_deinit(firm_err == 1);
		
	// deinit filesystem
	fsUnmountAll();
	
	// free memory (should already be free at this time)
	if(err_string) free(err_string);
	
	// power off
	if(hidGetPowerButton(true) || (menu_ret == MENU_RET_POWEROFF))
	{
		storeBootslot(0);
		power_off();
	}
	
	// reboot
	if(menu_ret == MENU_RET_REBOOT)
	{
		storeBootslot(0);
		power_reboot();
	}

	// firm launch
	if(startFirmLaunch)
	{
		// make sure GFX is in the right state
		if(!gfx_initialized && (firm_err == 1))
		{
			GFX_init(true);
			GFX_deinit(true);
		}
		
		// launch firm
		firmLaunch();
	}
	
	
	// reaching here was never intended....
	panicMsg("Reached code end!");
	return 0;
}
