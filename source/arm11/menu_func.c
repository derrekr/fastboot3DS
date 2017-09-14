/*
 *   This file is part of fastboot 3DS
 *   Copyright (C) 2017 derrek, profi200, d0k3
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
#include "arm11/hardware/hid.h"
#include "arm11/console.h"
#include "arm11/fmt.h"

// we need a better solution for this later (!!!)
void updateScreens(void)
{
	GX_textureCopy((u64*)RENDERBUF_TOP, 0, (u64*)GFX_getFramebuffer(SCREEN_TOP),
	               0, SCREEN_SIZE_TOP + SCREEN_SIZE_SUB);
	GFX_swapFramebufs();
	GFX_waitForEvent(GFX_EVENT_PDC0, true); // VBlank
}

u32 DummyFunc(PrintConsole* con, u32 param)
{
	// clear console
	consoleSelect(con);
	consoleClear();
	
	// print something
	ee_printf("Hello! I'm a dummy function.\nparam was %lu\n\nPress B to quit.\n\n", param);
	updateScreens();
	
	// wait for B button
	do
	{
		if(hidGetPowerButton(false)) // handle power button
		{
			ee_printf("POWER button pressed.\nPress A to confirm cancel & poweroff.\n");
			updateScreens();
			
			do
			{
				hidScanInput();
			}
			while (!(hidKeysDown() & (KEY_A|KEY_B)));
			
			if (hidKeysDown() & KEY_B)
			{
				ee_printf("POWER off canceled, continuing...\n\n");
				updateScreens();
				hidGetPowerButton(true);
			}
			else
			{
				return 1;
			}
		}
		hidScanInput();
	}
	while (!(hidKeysDown() & KEY_B));
	
	return 0;
}
