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
#include "types.h"
#include "arm11/menu.h"
#include "arm11/hardware/hid.h"
#include "arm11/console.h"
#include "arm11/fmt.h"
#include "hardware/gfx.h"



void menuShowDesc(MenuInfo* curr_menu, PrintConsole* desc_con, u32 index)
{
	MenuEntry* entry = &(curr_menu->entries[index]);
	char* desc = entry->desc;
	
	// select and clear description console
	consoleSelect(desc_con);
	consoleClear();
	
	// done if no description available
	if (!desc)
	{
		return;
	}
	
	const u32 desc_width = strlen(desc); // needs improvement (could be too large!)
	const u32 desc_height = 1; // need improvement
	int desc_x = (desc_con->consoleWidth - desc_width) >> 1;
	int desc_y = (desc_con->consoleHeight - desc_height) >> 1;
	
	// show description on screen
	consoleSetCursor(desc_con, desc_x, desc_y++);
	ee_printf(desc);
}

/**
 * @brief Draws the menu to the given console.
 * @param curr_menu A struct containing info about the current menu.
 * @param menu_con Console that the menu is displayed on.
 * @param index Current placement of the cursor.
 * @param is_sub_menu True if this is not the main menu.
 */
void menuDraw(MenuInfo* curr_menu, PrintConsole* menu_con, u32 index, bool is_sub_menu)
{
	const u32 menu_block_height = curr_menu->n_entries + 5;
	int menu_x = (menu_con->consoleWidth - MENU_WIDTH) >> 1;
	int menu_y = (menu_con->consoleHeight - menu_block_height) >> 1;
	
	// select menu console
	consoleSelect(menu_con);
	consoleClear();
	
	// menu title
	consoleSetCursor(menu_con, menu_x, menu_y++);
	ee_printf(curr_menu->name);
	
	// first separator
	consoleSetCursor(menu_con, menu_x, menu_y++);
	ee_printf("%*.*s", MENU_WIDTH, MENU_WIDTH, "============================================================");
	
	// menu entries
	for (u32 i = 0; i < curr_menu->n_entries; i++)
	{
		MenuEntry* entry = &(curr_menu->entries[i]);
		char* name = entry->name;
		bool is_selected = (i == index);
		
		consoleSetCursor(menu_con, menu_x, menu_y++);
		ee_printf(is_selected ? "[%s]" : " %s ", name);
	}
	
	// second separator
	consoleSetCursor(menu_con, menu_x, menu_y++);
	ee_printf("%*.*s", MENU_WIDTH, MENU_WIDTH, "============================================================");
	
	// button descriptions (A/B)
	consoleSetCursor(menu_con, menu_x, menu_y++);
	ee_printf(is_sub_menu ? "A: Choose  B: Return" : "A: Choose");
	
	// button descriptions (START)
	consoleSetCursor(menu_con, menu_x, menu_y++);
	ee_printf("START:  Reboot / [+\x1B] Poweroff");
	
	
}

/**
 * @brief Processes and displays the menu and user input.
 * @param info A complete description of the menu.
 * @param menu_con Console that the menu is displayed on.
 * @param desc_con Console that the description is displayed on.
 */
u32 menuProcess(MenuInfo* info)
{
	MenuInfo* curr_menu = info;
	MenuInfo* prev_menu[MENU_MAX_DEPTH];
	u32 prev_index[MENU_MAX_DEPTH];
	u32 menu_lvl = 0;
	u32 index = 0;
	u32 result = MENU_EXIT_REBOOT;
	
	// init menu console
	PrintConsole menu_con;
	consoleInit(SCREEN_TOP, &menu_con, true);
	
	// init description console
	PrintConsole desc_con;
	consoleInit(SCREEN_SUB, &desc_con, true);
	
	// draw menu & description for the first time
	menuDraw(curr_menu, &menu_con, 0, false);
	menuShowDesc(curr_menu, &desc_con, 0);
	
	// main menu processing loop
	while (true) {
		hidScanInput();
		const u32 kDown = hidKeysDown();
		const u32 kHeld = hidKeysHeld();
		bool redraw_menu = false;
		bool redraw_desc = false;
		
		if ((kDown & KEY_A) && (curr_menu->entries[index].function == NULL))
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
			(*(entry->function))(entry->param);
		}
		else if ((kDown & KEY_B) && (menu_lvl > 0))
		{
			// return to previous menu
			menu_lvl--;
			curr_menu = prev_menu[menu_lvl];
			index = prev_index[menu_lvl];
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
		if (kDown & KEY_START)
		{
			result = (kHeld & KEY_DLEFT) ? MENU_EXIT_POWEROFF : MENU_EXIT_REBOOT;
			break;
		}

		// update menu and description
		menuDraw(curr_menu, &menu_con, index, menu_lvl);
		menuShowDesc(curr_menu, &desc_con, index);


		GX_textureCopy((u64*)RENDERBUF_TOP, (240 * 2)>>4, (u64*)GFX_getFramebuffer(SCREEN_TOP),
		               (240 * 2)>>4, SCREEN_SIZE_TOP + SCREEN_SIZE_SUB);
		GFX_swapFramebufs();
		GFX_waitForEvent(GFX_EVENT_PDC0, true); // VBlank
	}

	return result;
}
