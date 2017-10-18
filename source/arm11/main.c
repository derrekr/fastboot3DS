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

#include "types.h"
#include "arm11/hardware/hardware.h"
#include "arm11/hardware/interrupt.h"
#include "arm11/hardware/hid.h"
#include "arm11/power.h"
#include "arm11/hardware/i2c.h"
#include "arm11/menu/menu.h"
#include "arm11/menu/menu_fb3ds.h"
#include "hardware/pxi.h"
#include "hardware/gfx.h"
#include "arm11/firm.h"
#include "arm11/console.h"
#include "arm11/fmt.h"
#include "fsutils.h"
#include "arm11/config.h"


volatile bool g_continueBootloader = true;
volatile bool g_startFirmLaunch = false;



int main(void)
{
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
		for(u32 i = 0; (i < 3) && !g_startFirmLaunch; i++)
		{
			if(!configDataExist(KBootOption1 + i) ||
				!configDataExist(KBootOption1Buttons + i))
				continue;
			
			u32 combo = *(u32*) configGetData(KBootOption1Buttons + i);
			if((kHeld & 0xfff) == combo)
			{
				char* path = (char*) configGetData(KBootOption1 + i);
				g_startFirmLaunch = (loadVerifyFirm(path, false) >= 0);
			}
		}
	}
	
	// get bootmode from config
	u32 bootmode = BootModeNormal;
	if(configDataExist(KBootMode))
		bootmode = *(u32*) configGetData(KBootMode);

	// menu specific code
	// HOME button detection may not work yet
	if(!g_startFirmLaunch && ((bootmode == BootModeNormal) || hidIsHomeButtonHeldRaw()))
	{
		// init screens
		GFX_init();
		
		// init menu console
		PrintConsole menu_con;
		consoleInit(SCREEN_SUB, &menu_con, false);

		// init description console
		PrintConsole desc_con;
		consoleInit(SCREEN_TOP, &desc_con, false);

		// run menu
		g_continueBootloader = false;
		menuProcess(&menu_con, &desc_con, menu_fb3ds);
		
		// write config (if something changed)
		if (configHasChanged()) writeConfigFile();
		
		// deinit GFX
		GFX_deinit();
	}
	
	// search for a bootable firmware (all slots)
	if (g_continueBootloader && !g_startFirmLaunch)
	{
		for(u32 i = 0; (i < 3) && !g_startFirmLaunch; i++)
		{
			if(!configDataExist(KBootOption1 + i))
				continue;
			
			char* path = (char*) configGetData(KBootOption1 + i);
			g_startFirmLaunch = (loadVerifyFirm(path, false) >= 0);
		}
	}
	
	// deinit filesystem
	fsUnmountAll();

	// firm launch
	if(g_startFirmLaunch) firmLaunch();
	
	// power off
	power_off();
	
	return 0;
}
