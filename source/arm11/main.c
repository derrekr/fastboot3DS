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
#include "arm11/hardware.h"
#include "arm11/interrupt.h"
#include "arm11/hid.h"
#include "arm11/power.h"
#include "arm11/i2c.h"
#include "pxi.h"
#include "gfx.h"
#include "arm11/firm.h"


volatile bool g_poweroffAllowed = false;
volatile bool g_startFirmLaunch = false;



int main(void)
{
	hardwareInit();

	while(!g_startFirmLaunch)
	{
		waitForInterrupt();

		hidScanInput();
		const u32 kDown = hidKeysDown();
		const u32 kUp = hidKeysUp();

		if(hidGetPowerButton())
		{
			if(g_poweroffAllowed) power_off();
			PXI_trySendWord(PXI_RPL_POWER_PRESSED);
		}

		if(kDown & KEY_HOME) PXI_trySendWord(PXI_RPL_HOME_PRESSED);
		if(kDown & KEY_SHELL) i2cmcu_lcd_poweroff();
		if(kUp & KEY_SHELL)
		{
			i2cmcu_lcd_poweron();
			i2cmcu_lcd_backlight_poweron();
		}


		u8 hidstate = i2cmcu_readreg_hid_held();
		
		if(!(hidstate & MCU_HID_HOME_BUTTON_NOT_HELD))
			PXI_trySendWord(PXI_RPL_HOME_HELD);
	}

	GFX_deinit(); // TODO: Let ARM9 decide when to deinit gfx

	firm_launch();

	return 0;
}
