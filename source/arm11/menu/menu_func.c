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
#include "arm11/menu/menu_fsel.h"
#include "arm11/menu/menu_util.h"
#include "arm11/hardware/hid.h"
#include "arm11/console.h"
#include "arm11/config.h"
#include "arm11/fmt.h"
#include "arm11/firm.h"
#include "arm11/main.h"



void outputEndWait(void)
{
	u32 kDown = 0;
	
	do
	{
		GFX_waitForEvent(GFX_EVENT_PDC0, true);
		
		if(hidGetPowerButton(false)) // handle power button
			break;
		
		hidScanInput();
		kDown = hidKeysDown();
		if (kDown & (KEY_SHELL)) sleepmode();
	}
	while (!(kDown & (KEY_B|KEY_HOME)));
}

void updateScreens(void)
{
	GX_textureCopy((u64*)RENDERBUF_TOP, 0, (u64*)GFX_getFramebuffer(SCREEN_TOP),
	               0, SCREEN_SIZE_TOP + SCREEN_SIZE_SUB);
	GFX_swapFramebufs();
	GFX_waitForEvent(GFX_EVENT_PDC0, true); // VBlank
}

u32 menuPresetBootMode(void)
{
	if (configDataExist(KBootMode))
	{
		return (1 << (*(u32*) configGetData(KBootMode)));
	}
		
	return 0;
}

u32 menuPresetBootSlot(void)
{
	u32 res = 0;
	
	for (u32 i = 0; i < 3; i++)
	{
		if (configDataExist(KBootOption1 + i))
		{
			res |= 1 << i;
		}
	}
	
	return res;
}

u32 menuSetBootMode(PrintConsole* con, u32 param)
{
	(void) con;
	u32 res = (configSetKeyData(KBootMode, &param)) ? 0 : 1;
	
	return res;
}

u32 menuSetupBootSlot(PrintConsole* con, u32 param)
{
	bool slot = param & 0xF;
	char res_path[256];
	char* start = NULL;
	
	// if bit4 of param is set, reset slot and return
	if (param & 0x10)
	{
		configDeleteKey(KBootOption1Buttons + slot);
		configDeleteKey(KBootOption1 + slot);
		return 0;
	}
	
	if (configDataExist(KBootOption1 + slot))
		start = (char*) configGetData(KBootOption1 + slot);
	
	u32 res = 0;
	if (menuFileSelector(res_path, con, start, "*.firm"))
		res = (configSetKeyData(KBootOption1 + slot, res_path)) ? 0 : 1;
	
	return res;
}

static const char * convTable[] = {
	"A", "B", "SELECT", "START", "RIGHT", "LEFT",
	"UP", "DOWN", "R", "L", "X", "Y"
};

u32 menuSetupBootKeys(PrintConsole* con, u32 param)
{
	const u32 y_center = 7;
	const u32 y_instr = 21;
	
	hidScanInput();
	u32 kHeld = hidKeysHeld();
	
	while (true)
	{
		// build button string
		char button_str[80];
		char* ptr = button_str;
		bool first = true;
		for (u32 i = 0; i < 12; i++)
		{
			if (kHeld & (1<<i))
			{
				ptr += ee_sprintf(ptr, "%s[%s]", first ? " " : "+", convTable[i]);
				first = false;
			}
		}
		if (first) // backup solution for no buttons
			ee_sprintf(ptr, "(no buttons)");
		
		// clear console
		consoleSelect(con);
		consoleClear();
		
		// draw input block
		con->cursorY = y_center;
		ee_printf_line_center("Hold button(s) to setup.");
		ee_printf_line_center("Currently held buttons:");
		ee_printf_line_center(button_str);
		
		// draw instructions
		con->cursorY = y_instr;
		if (configDataExist(KBootOption1Buttons + param))
		{
			char* currentSetting =
				(char*) configCopyText(KBootOption1Buttons + param);
			ee_printf_line_center("Current: %s", currentSetting);
			free(currentSetting);
		}
		ee_printf_line_center("[HOME] to cancel");
		
		// update screens
		updateScreens();
		
		// check for buttons until held for ~1.5sec
		u32 kHeldNew = 0;
		do
		{
			// check hold duration
			u32 vBlanks = 0;
			do
			{
				GFX_waitForEvent(GFX_EVENT_PDC0, true);
				if(hidGetPowerButton(false)) return 1;
				
				hidScanInput();
				kHeldNew = hidKeysHeld();
				if(hidKeysDown() & KEY_SHELL) sleepmode();
			}
			while ((kHeld == kHeldNew) && (++vBlanks < 100));
			
			// check HOME key
			if (kHeldNew & KEY_HOME) return 1;
		}
		while (!((kHeld|kHeldNew) & 0xfff));
		// repeat checks until actual buttons are held
		
		if (kHeld == kHeldNew) break;
		kHeld = kHeldNew;
	}
	
	// if we arrive here, we have a button combo
	u32 res = (configSetKeyData(KBootOption1Buttons + param, &kHeld)) ? 0 : 1;
	
	return res;
}

u32 menuLaunchFirm(PrintConsole* con, u32 param)
{
	char path_store[256];
	char* path;
	
	if (param < 3) // loading from bootslot
	{
		// clear console
		consoleSelect(con);
		consoleClear();
	
		// check if bootslot exists
		ee_printf("Checking bootslot...\n");
		if (!configDataExist(KBootOption1 + param))
		{
			ee_printf("Bootslot does not exist!\n");
			goto fail;
		}
		path = (char*) configGetData(KBootOption1 + param);
	}
	else if (param == 0xFE) // loading from FIRM1
	{
		path = "firm1:";
	}
	else if (param == 0xFF) // user decision
	{
		path = path_store;
		if (!menuFileSelector(path, con, NULL, "*.firm"))
			return 1;
	}
	
	// clear console
	if (param >= 3)
	{
		consoleSelect(con);
		consoleClear();
	}
	
	// try load and verify
	ee_printf("\nLoading %s...\n", path);
	s32 res = loadVerifyFirm(path, false);
	if (res != 0)
	{
		ee_printf("Firm %s error code %li!\n", (res > -8) ? "load" : "verify", res);
		goto fail;
	}
	
	ee_printf("\nFirm load success, launching firm..."); // <-- you will never see this
	g_startFirmLaunch = true;
	
	return 0;
	
	fail:
	
	ee_printf("\nFirm launcher failed.\n\nPress B or HOME to return.");
	updateScreens();
	outputEndWait();
	
	return 1;
}

u32 menuContinueBoot(PrintConsole* con, u32 param)
{
	// all the relevant stuff handled outside
	(void) con;
	(void) param;
	g_continueBootloader = true;
	return 0;
}

u32 menuDummyFunc(PrintConsole* con, u32 param)
{
	(void) param;
	
	// clear console
	consoleSelect(con);
	consoleClear();
	
	// print something
	ee_printf("\nThis is not implemented yet.\nGo look elsewhere, nothing to see here.\n\nPress B or HOME to return.");
	updateScreens();
	outputEndWait();

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
			free(text);
		}
	}
	updateScreens();
	
	// wait for B / HOME button
	do
	{
		GFX_waitForEvent(GFX_EVENT_PDC0, true);
		if(hidGetPowerButton(false)) // handle power button
			return 0;
		
		hidScanInput();
	}
	while (!(hidKeysDown() & (KEY_B|KEY_HOME)));
	
	return 0;
}

u32 menuShowCredits(PrintConsole* con, u32 param)
{
	(void) param;
	
	// clear console
	consoleSelect(con);
	consoleClear();
	
	// credits
	con->cursorY = 4;
	ee_printf_line_center("Fastboot3DS Credits");
	ee_printf_line_center("===================");
	ee_printf_line_center("");
	ee_printf_line_center("Main developers:");
	ee_printf_line_center("derrek");
	ee_printf_line_center("profi200");
	ee_printf_line_center("");
	ee_printf_line_center("Thanks to:");
	ee_printf_line_center("yellows8");
	ee_printf_line_center("plutoo");
	ee_printf_line_center("smea");
	ee_printf_line_center("Normmatt (for sdmmc code)");
	ee_printf_line_center("WinterMute (for console code)");
	ee_printf_line_center("d0k3 (for menu code)");
	ee_printf_line_center("");
	ee_printf_line_center("... everyone who contributed to 3dbrew.org");
	updateScreens();

	// wait for user
	outputEndWait();
	
	return 0;
}
