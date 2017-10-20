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
#include "arm11/menu/menu.h"
#include "arm11/menu/menu_util.h"
#include "arm11/hardware/hid.h"
#include "arm11/config.h"
#include "arm11/console.h"
#include "arm11/fmt.h"
#include "arm11/main.h"
#include "hardware/gfx.h"



void menuBuildDescString(char* desc, u32 slot, const char* name_raw, const char* desc_raw)
{
	// using no slot description
	if (slot >= 3)
	{
		ee_snprintf(desc, 512, "%s:\n%s", name_raw, desc_raw);
		stringWordWrap(desc, WORDWRAP_WIDTH);
		return;
	}
	
	
	// raw description plus slot description
	// get strings for path and keycombo
	char slot_path_store[24+1];
	char* slot_path = NULL;
	char* keycombo = NULL;
	
	if(configDataExist(KBootOption1 + slot))
	{
		slot_path = slot_path_store;
		truncateString(slot_path, (char*) configGetData(KBootOption1 + slot), 24, 8);
	}
	
	if(configDataExist(KBootOption1Buttons + slot))
		keycombo = (char*) configCopyText(KBootOption1Buttons + slot);
	
	
	// write description to desc string
	ee_snprintf(desc, 512, "%s:\n%s\n \npath: %s\nkeys: %s",
		name_raw, desc_raw,
		slot_path ? slot_path : "- not set up -",
		keycombo ? keycombo : "- not set up -");
	
	// wordwrap description string
	stringWordWrap(desc, WORDWRAP_WIDTH);
	
	
	if (keycombo) free(keycombo);
}
	
void menuShowDesc(MenuInfo* curr_menu, PrintConsole* desc_con, u32 index)
{
	// select and clear description console
	consoleSelect(desc_con);
	consoleClear();
	
	// print title at the top
	const char* title = "fastboot 3DS " VERS_STRING;
	consoleSetCursor(desc_con, (desc_con->consoleWidth - strlen(title)) >> 1, 1);
	ee_printf(title);
	
	// get description, check if available
	MenuEntry* entry = &(curr_menu->entries[index]);
	char* name = entry->name;
	char* desc = entry->desc;
	
	// done if no description available
	if (!name || !desc)
		return;
	
	// build description string
	// also handle MENU_FLAG_SLOTS flag
	char desc_ww[512];
	u32 desc_slot = (u32) -1;
	if (curr_menu->flags & 0xF) // includes all known flags
	{
		// flags only concern descriptions right now
		desc_slot = (curr_menu->flags & MENU_FLAG_SLOTS) ? index :
			(curr_menu->flags & MENU_FLAG_SLOT(1)) ? 0 :
			(curr_menu->flags & MENU_FLAG_SLOT(2)) ? 1 :
			(curr_menu->flags & MENU_FLAG_SLOT(3)) ? 2 : (u32) -1;
	}
	menuBuildDescString(desc_ww, desc_slot, name, desc);
	
	// get width, height
	int desc_width = stringGetWidth(desc_ww);
	int desc_height = stringGetHeight(desc_ww);
	desc_width = (desc_width > desc_con->consoleWidth) ? desc_con->consoleWidth : desc_width;
	desc_height = (desc_height > desc_con->consoleHeight) ? desc_con->consoleHeight : desc_height;
	
	// write to console
	int desc_x = (desc_con->consoleWidth - desc_width) >> 1;
	int desc_y = (desc_con->consoleHeight - 1 - desc_height);
	
	for (char* str = strtok(desc_ww, "\n"); str != NULL; str = strtok(NULL, "\n")) {
		consoleSetCursor(desc_con, desc_x, desc_y++);
		ee_printf(str);
	}
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
	int menu_x = (menu_con->consoleWidth - MENU_WIDTH) >> 1;
	// int menu_y = MENU_DISP_Y + ((menu_con->consoleHeight - menu_block_height) >> 1);
	int menu_y = MENU_OFFSET_TITLE;
	int menu_preset = 0;
	
	// get menu preset
	if (curr_menu->preset)
		menu_preset = curr_menu->preset();
	
	// select menu console
	consoleSelect(menu_con);
	consoleClear();
	
	// menu title
	consoleSetCursor(menu_con, menu_x, menu_y++);
	ee_printf(curr_menu->name);
	menu_y++;
	
	// menu entries
	for (u32 i = 0; i < curr_menu->n_entries; i++)
	{
		MenuEntry* entry = &(curr_menu->entries[i]);
		char* name = entry->name;
		bool is_selected = (i == index);
		
		consoleSetCursor(menu_con, menu_x, menu_y++);
		if (is_selected) ee_printf("\x1b[47;30m");
		ee_printf("%.3s %-*.*s", ((menu_preset >> i) & 0x1) ? "[X]" : "[ ]", MENU_WIDTH-4, MENU_WIDTH-4, name);
		ee_printf("\x1b[0m");
	}
	
	// button instructions
	menu_y = MENU_OFFSET_BUTTONS;
	consoleSetCursor(menu_con, menu_x, menu_y++);
	ee_printf("%-*.*s", MENU_WIDTH, MENU_WIDTH, "[A]:Choose [B]:Back");
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
	
	// main menu processing loop
	while (!g_startFirmLaunch && !g_continueBootloader) {
		// update menu and description (on demand)
		if ((index != last_index) || (curr_menu != last_menu)) {
			menuDraw(curr_menu, menu_con, index);
			menuShowDesc(curr_menu, desc_con, index);
			last_index = index;
			last_menu = curr_menu;

			GX_textureCopy((u64*)RENDERBUF_TOP, 0, (u64*)GFX_getFramebuffer(SCREEN_TOP),
						0, SCREEN_SIZE_TOP + SCREEN_SIZE_SUB);
			GFX_swapFramebufs();
		}
		GFX_waitForEvent(GFX_EVENT_PDC0, true); // VBlank
		
		// handle power button
		if(hidGetPowerButton(false))
			break; // deinits & poweroff outside of this function
		
		hidScanInput();
		const u32 kDown = hidKeysDown();
		// const u32 kHeld = hidKeysHeld();
		
		if (kDown & KEY_SHELL)
		{
			sleepmode();
		}
		else if ((kDown & KEY_A) && (curr_menu->entries[index].function == NULL))
		{
			// store previous menu and index for return
			if (menu_lvl < MENU_MAX_DEPTH)
			{
				prev_menu[menu_lvl] = curr_menu;
				prev_index[menu_lvl] = index;
				menu_lvl++;
			}
			// enter submenu
			curr_menu = info + curr_menu->entries[index].param;
			index = 0;
		}
		else if (kDown & KEY_A)
		{
			// call menu entry function
			MenuEntry* entry = &(curr_menu->entries[index]);
			(*(entry->function))(desc_con, entry->param);
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

	return 0;
}
