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



#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "arm11/hardware/hid.h"
#include "arm11/hardware/mcu.h"
#include "arm11/menu/battery.h"
#include "arm11/menu/bootinfo.h"
#include "arm11/menu/bootslot.h"
#include "arm11/menu/menu.h"
#include "arm11/menu/menu_util.h"
#include "arm11/menu/menu_color.h"
#include "arm11/config.h"
#include "arm11/console.h"
#include "arm11/debug.h"
#include "arm11/fmt.h"
#include "fs.h" // for drive names

static BatteryState battery;

void menuBuildDescString(char* desc, u32 flags, u32 index, const char* desc_raw)
{
	char* desc_ptr = desc;
	
	// copy raw description
	desc_ptr += ee_sprintf(desc_ptr, desc_raw);
	
	
	// handle flags
	u32	slot = -1;
	if (flags & MENU_FLAG_SLOTS)
	{
		slot = index;
	}
	else
	{
		MENU_GET_FLAGSLOT(slot, flags);
		slot--;
	}
	
	// boot mode description
	if ((flags & MENU_FLAG_BOOTMODE) && (index == N_BOOTSLOTS))
	{
		if (configDataExist(KBootMode))
		{
			char* modestr = (char*) configCopyText(KBootMode);
			if (!modestr) panicMsg("Config error");
			desc_ptr += ee_sprintf(desc_ptr, "\n \ncurrent: %s", modestr);
			free (modestr);
		}
		else desc_ptr += ee_sprintf(desc_ptr, "\n \ncurrent: - not set up -");
	}
	// boot slot description
	else if (slot < N_BOOTSLOTS)
	{
		// get strings for path and keycombo
		char slot_path_store[40+1];
		char* slot_path = NULL;
		char* keycombo = NULL;
		
		if(configDataExist(KBootOption1 + slot))
		{
			slot_path = slot_path_store;
			truncateString(slot_path, (char*) configGetData(KBootOption1 + slot), 40, 8);
		}
		
		if(configDataExist(KBootOption1Buttons + slot))
			keycombo = (char*) configCopyText(KBootOption1Buttons + slot);
		
		// write description to desc string
		desc_ptr += ee_sprintf(desc_ptr, "\n \npath: %s\nboot: %s",
			slot_path ? slot_path : "- slot disabled -",
			!slot_path ? "- slot disabled -" : keycombo ? keycombo : "bootloader (auto)");
		
		if (keycombo) free(keycombo);
	}
	
	
	// wordwrap the whole string
	stringWordWrap(desc, WORDWRAP_WIDTH);
}
	
void menuShowDesc(MenuInfo* curr_menu, PrintConsole* desc_con, u32 index)
{
	// select and clear description console
	consoleSelect(desc_con);
	consoleClear();
	
	
	// print title at the top
#ifdef NDEBUG
	const char* title = "fastboot3DS " VERS_STRING;
#else
	const char* title = "fastboot3DS " VERS_STRING " (debug)";
#endif
	consoleSetCursor(desc_con, (desc_con->windowWidth - strlen(title)) >> 1, 0);
	ee_printf(ESC_SCHEME_ACCENT0 "%s" ESC_RESET, title);
	
	
	// get bootinfo
	const char* mount_paths[] = { FS_DRIVE_NAMES };
	bootInfo bootinfo;
	getBootInfo(&bootinfo);
	
	// print bootinfo
	consoleSetCursor(desc_con, 0, 3);
	ee_printf("Model: %s\n", bootinfo.model);
	ee_printf("%s\n", bootinfo.bootEnv);
	ee_printf("Battery: %s%lu%s %%" ESC_RESET "\n\n",
		((battery.percent <= 20) && !battery.charging) ? ESC_SCHEME_BAD : ESC_SCHEME_GOOD,
		battery.percent, (battery.charging) ? "+" : "");
	for (u32 i = 0; i < sizeof(mount_paths) / sizeof(const char*); i++)
	{
		ee_printf(ESC_SCHEME_WEAK "%s" ESC_RESET " %s\n", mount_paths[i],
			((bootinfo.mountState >> i) & 0x1) ?
			 ESC_SCHEME_GOOD "mounted" ESC_RESET :
			 ESC_SCHEME_BAD "not mounted" ESC_RESET);
	}
	
	
	// print config
	u32 conw = desc_con->windowWidth;
	u32 curY = 3;
	
	// print config location
	consoleSetCursor(desc_con, conw - 16, curY++);
	if(!configIsLoaded())
	{
		ee_printf("Config not found");
	}
	else
	{
		FsDevice loc_conf = configGetStorageLocation();
		ee_printf("Config from %s", (loc_conf == FS_DEVICE_SDMC) ? "SDMC" : "NAND");
	}
	
	// print boot mode
	consoleSetCursor(desc_con, conw - 16, curY++);
	if(configDataExist(KBootMode))
	{
		char* bootmode = configCopyText(KBootMode);
		ee_printf("%7.7s bootmode", bootmode);
		if (bootmode) free(bootmode);
	}
	else
	{
		ee_printf("bootmode not set");
	}
	
	// print slot config
	curY++;
	for(u32 i = 0; i < N_BOOTSLOTS; i++)
	{
		consoleSetCursor(desc_con, conw - 16, curY++);
		ee_printf(ESC_SCHEME_WEAK "slot %lu: " ESC_RESET, i + 1);
		if(configDataExist(KBootOption1 + i))
			ee_printf((configDataExist(KBootOption1Buttons + i)) ? "keycombo" : "autoboot");
		else ee_printf("   -    ");
	}
	
	// reboot slot
	u32 prevSlot = readStoredBootslot();
	consoleSetCursor(desc_con, conw - 16, curY++);
	ee_printf(ESC_SCHEME_WEAK "reboot: " ESC_RESET);
	if ((prevSlot >= 1) && (prevSlot <= N_BOOTSLOTS))
		ee_printf(" slot %lu ", prevSlot);
	else
		ee_printf("autoboot");
	
	// print dev mode config
	curY++;
	if(configDevModeEnabled())
	{
		consoleSetCursor(desc_con, conw - 14, curY);
		ee_printf(ESC_SCHEME_BAD "devmode active" ESC_RESET);
	}
	
	
	// get description, check if available
	MenuEntry* entry = &(curr_menu->entries[index]);
	char* name = entry->name;
	char* desc = entry->desc;
	
	// done if no description available
	if (!name || !desc)
		return;
	
	// build description string
	char desc_ww[512];
	menuBuildDescString(desc_ww, curr_menu->flags, index, desc);
	
	// get width, height
	int desc_width = stringGetWidth(desc_ww);
	int desc_height = stringGetHeight(desc_ww);
	desc_width = (desc_width > desc_con->windowWidth) ? desc_con->windowWidth : desc_width;
	desc_height = (desc_height > desc_con->windowHeight) ? desc_con->windowHeight : desc_height;
	
	// write to console
	int desc_x = (desc_con->windowWidth - desc_width) >> 1;
	int desc_y = (desc_con->windowHeight - 1 - desc_height);
	
	consoleSetCursor(desc_con, desc_x, desc_y++);
	ee_printf(ESC_SCHEME_ACCENT1 "%s:\n" ESC_RESET, name);
	
	ee_printf(ESC_SCHEME_WEAK);
	for (char* str = strtok(desc_ww, "\n"); str != NULL; str = strtok(NULL, "\n"))
	{
		consoleSetCursor(desc_con, desc_x, desc_y++);
		ee_printf(str);
	}
	ee_printf(ESC_RESET);
}

/**
 * @brief Draws the menu to the given console.
 * @param curr_menu A struct containing info about the current menu.
 * @param menu_con Console that the menu is displayed on.
 * @param index Current placement of the cursor.
 */
void menuDraw(MenuInfo* curr_menu, PrintConsole* menu_con, u32 index)
{
	// const u32 menu_block_height = curr_menu->n_entries + 5; // +5 for title and button instructions
	int menu_x = (menu_con->windowWidth - MENU_WIDTH) >> 1;
	// int menu_y = MENU_DISP_Y + ((menu_con->windowHeight - menu_block_height) >> 1);
	int menu_y = MENU_OFFSET_TITLE;
	int menu_preset = 0xFF;
	
	// get menu preset
	if (curr_menu->preset)
		menu_preset = curr_menu->preset();
	
	// select menu console
	consoleSelect(menu_con);
	consoleClear();
	
	// menu title
	consoleSetCursor(menu_con, menu_x, menu_y++);
	ee_printf(ESC_SCHEME_ACCENT1 "%s" ESC_RESET, curr_menu->name);
	menu_y++;
	
	// menu entries
	for (u32 i = 0; i < curr_menu->n_entries; i++)
	{
		MenuEntry* entry = &(curr_menu->entries[i]);
		char* name = entry->name;
		bool is_selected = (i == index);
		
		char symbol = ' ';
		if (curr_menu->flags & MENU_FLAG_CONFIG)
			symbol = ((menu_preset >> i) & 0x1) ? '\xFB' : ' ';
		else
			symbol = ((menu_preset >> i) & 0x1) ? '+' : '-';
		
		consoleSetCursor(menu_con, menu_x, menu_y++);
		// ee_printf(((menu_preset >> i) & 0x1) ? ESC_SCHEME_STD : ESC_SCHEME_WEAK);
		ee_printf(ESC_SCHEME_STD);
		if (is_selected) ee_printf(ESC_INVERT);
		ee_printf("[%c] %-*.*s", symbol, MENU_WIDTH-4, MENU_WIDTH-4, name);
		ee_printf(ESC_RESET);
	}
	
	// button instructions
	menu_y = MENU_OFFSET_BUTTONS;
	consoleSetCursor(menu_con, menu_x, menu_y++);
	ee_printf(ESC_SCHEME_WEAK "%-*.*s" ESC_RESET, MENU_WIDTH, MENU_WIDTH, "[A]:Choose [B]:Back");
}

/**
 * @brief Processes and displays the menu and user input.
 * @param menu_con Console that the menu is displayed on.
 * @param desc_con Console that the description is displayed on.
 * @param info A complete description of the menu.
 */
u32 menuProcess(PrintConsole* menu_con, PrintConsole* desc_con, MenuInfo* info)
{
	MenuInfo* curr_menu = info;
	MenuInfo* last_menu = NULL;
	MenuInfo* prev_menu[MENU_MAX_DEPTH];
	u32 prev_index[MENU_MAX_DEPTH];
	u32 menu_lvl = 0;
	u32 index = 0;
	u32 last_index = (u32) -1;
	
	u64 n_vblanks = 0;
	getBatteryState(&battery);
	
	// main menu processing loop
	u32 menu_ret = MENU_OK;
	while (menu_ret < MENU_RET_CONTINUE)
	{
		// handle power button
		if(hidGetPowerButton(false))
			break; // deinits & poweroff outside of this function
		
		// handle battery state
		if ((++n_vblanks % 250) == 0)
			getBatteryState(&battery);
		
		// update menu and description (on demand)
		if ((index != last_index) || (curr_menu != last_menu) || (n_vblanks % 250 == 0)) {
			menuDraw(curr_menu, menu_con, index);
			menuShowDesc(curr_menu, desc_con, index);
			last_index = index;
			last_menu = curr_menu;
			updateScreens(); // update screens (VBlank included)
		} else GFX_waitForEvent(GFX_EVENT_PDC0, true); // VBlank
		
		hidScanInput();
		const u32 kDown = hidKeysDown();
		// const u32 kHeld = hidKeysHeld();
		
		if (kDown & KEY_SHELL)
		{
			sleepmode();
		}
		else if ((kDown & KEY_A) && (curr_menu->entries[index].function == NULL))
		{
			u32 next_menu_idx = curr_menu->entries[index].param;
			
			// store previous menu and index for return
			if (menu_lvl < MENU_MAX_DEPTH)
			{
				prev_menu[menu_lvl] = curr_menu;
				prev_index[menu_lvl] = index;
				menu_lvl++;
			}
			// enter submenu
			curr_menu = info + next_menu_idx;
			index = 0;
		}
		else if (kDown & KEY_A)
		{
			// call menu entry function
			MenuEntry* entry = &(curr_menu->entries[index]);
			menu_ret = (*(entry->function))(desc_con, menu_con, entry->param);
			// force redraw (somewhat hacky)
			last_menu = NULL;
		}
		else if ((kDown & KEY_B) && (menu_lvl > 0))
		{
			// return to previous menu
			menu_lvl--;
			curr_menu = prev_menu[menu_lvl];
			index = prev_index[menu_lvl];
		}
		else if ((kDown & KEY_HOME) && (menu_lvl > 0))
		{
			// return to HOME menu
			menu_lvl = 0;
			curr_menu = info;
			index = 0;
		}
		else if (kDown & KEY_DDOWN)
		{
			// cursor down
			index = (index == curr_menu->n_entries - 1) ? 0 : index + 1;
		}
		else if (kDown & KEY_DUP)
		{
			// cursor up
			index = (index == 0) ? curr_menu->n_entries - 1 : index - 1;
		}
	}

	return menu_ret;
}
