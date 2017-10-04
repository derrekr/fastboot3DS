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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "arm11/hardware/hid.h"
#include "arm11/console.h"
#include "arm11/config.h"
#include "arm11/fmt.h"

// we need a better solution for this later (!!!)
void updateScreens(void)
{
	GX_textureCopy((u64*)RENDERBUF_TOP, 0, (u64*)GFX_getFramebuffer(SCREEN_TOP),
	               0, SCREEN_SIZE_TOP + SCREEN_SIZE_SUB);
	GFX_swapFramebufs();
	GFX_waitForEvent(GFX_EVENT_PDC0, true); // VBlank
}

// doesn't belong in here, either
u32 ee_printf_line_center(const char *const fmt, ...)
{
	char buf[64];
	va_list args;
	va_start(args, fmt);
	ee_vsnprintf(buf, 64, fmt, args);
	va_end(args);
	
	PrintConsole* con = consoleGet();
	int pad = (con->consoleWidth - strlen(buf)) / 2;
	if (pad < 0) pad = 0;
	con->cursorX = 0;
	
	return ee_printf("%*.*s%s\n", pad, pad, "", buf);
}

u32 menuPresetBootMode(void)
{
	if (configDataExist(KBootMode))
	{
		return (1 << (*(u32*) configGetData(KBootMode)));
	}
		
	return 0;
}

u32 menuSetBootMode(PrintConsole* con, u32 param)
{
	(void) con;
	
	return (configSetKeyData(KBootMode, &param)) ? 0 : 1;
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

u32 SetView(PrintConsole* con, u32 param)
{
	(void) param;
	
	// clear console
	consoleSelect(con);
	consoleClear();
	
	ee_printf("Write config: %s\n", writeConfigFile() ? "success" : "failed");
	ee_printf("Load config: %s\n", loadConfigFile() ? "success" : "failed");
	
	// show settings
	for (int key = 0; key < KLast; key++)
	{
		const char* kText = configGetKeyText(key);
		const bool kExist = configDataExist(key);
		ee_printf("%02i %s: %s\n", key, kText, kExist ? "exists" : "not found");
		if (configDataExist(key))
		{
			char* text = (char*) configCopyText(key);
			ee_printf("text: %s / u32: %lu\n", text, *(u32*) configGetData(key));
			if (text) free(text);
		}
	}
	updateScreens();
	
	// wait for B button
	do
	{
		if(hidGetPowerButton(false)) // handle power button
		{
			return 1;
		}
		hidScanInput();
	}
	while (!(hidKeysDown() & KEY_B));
	
	return 0;
}

u32 ShowCredits(PrintConsole* con, u32 param)
{
	(void) param;
	
	// clear console
	consoleSelect(con);
	consoleClear();
	
	// credits
	con->cursorY = 4;
	ee_printf_line_center("Fastboot3DS Credits");	ee_printf_line_center("===================");	ee_printf_line_center("");	ee_printf_line_center("Main developers:");	ee_printf_line_center("derrek");	ee_printf_line_center("profi200");	ee_printf_line_center("");	ee_printf_line_center("Thanks to:");	ee_printf_line_center("yellows8");	ee_printf_line_center("plutoo");	ee_printf_line_center("smea");	ee_printf_line_center("Normmatt (for sdmmc code)");	ee_printf_line_center("WinterMute (for console code)");	ee_printf_line_center("d0k3 (for menu code)");	ee_printf_line_center("");	ee_printf_line_center("... everyone who contributed to 3dbrew.org");
	updateScreens();

	// wait for B button
	do
	{
		if(hidGetPowerButton(false)) // handle power button
		{
			return 1;
		}
		hidScanInput();
	}
	while (!(hidKeysDown() & KEY_B));
	
	return 0;
}
