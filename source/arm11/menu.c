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
#include "arm11/power.h"
#include "hardware/gfx.h"



u32 stringGetHeight(const char* str) {
	u32 height = 1;
	for (char* lf = strchr(str, '\n'); (lf != NULL); lf = strchr(lf + 1, '\n'))
		height++;
	return height;
}

u32 stringGetWidth(const char* str) {
	u32 width = 0;
	char* old_lf = (char*) str;
	char* str_end = (char*) str + strlen(str);
	for (char* lf = strchr(str, '\n'); lf != NULL; lf = strchr(lf + 1, '\n')) {
		if ((u32) (lf - old_lf) > width) width = lf - old_lf;
		old_lf = lf;
	}
	if ((u32) (str_end - old_lf) > width)
		width = str_end - old_lf;
	return width;
}

void stringWordWrap(char* str, int llen) {
	char* last_brk = str - 1;
	char* last_spc = str - 1;
	for (char* str_ptr = str;; str_ptr++) {
		if (!*str_ptr || (*str_ptr == ' ')) { // on space or string_end
			if (str_ptr - last_brk > llen) { // if maximum line lenght is exceeded
				if (last_spc > last_brk) { // put a line_brk at the last space
					*last_spc = '\n';
					last_brk = last_spc;
					last_spc = str_ptr;
				} else if (*str_ptr) { // if we have no applicable space
					*str_ptr = '\n';
					last_brk = str_ptr;
				}
			} else if (*str_ptr) last_spc = str_ptr;
		} else if (*str_ptr == '\n') last_brk = str_ptr;
		if (!*str_ptr) break;
	}
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
	{
		return;
	}
	
	
	// word wrap description string
	char desc_ww[512];
	strncpy(desc_ww, desc, 512);
	stringWordWrap(desc_ww, WORDWRAP_WIDTH);
	
	// get width, height
	int desc_width = stringGetWidth(desc_ww);
	int desc_height = stringGetHeight(desc_ww);
	desc_width = (desc_width > desc_con->consoleWidth) ? desc_con->consoleWidth : desc_width;
	desc_height = (desc_height > desc_con->consoleHeight) ? desc_con->consoleHeight : desc_height;
	
	// write to console
	int desc_x = (desc_con->consoleWidth - desc_width) >> 1;
	int desc_y = (desc_con->consoleHeight - 2 - desc_height);
	
	consoleSetCursor(desc_con, desc_x, desc_y++);
	ee_printf("%s:", name);
	
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
 * @param is_sub_menu True if this is not the main menu.
 */
void menuDraw(MenuInfo* curr_menu, PrintConsole* menu_con, u32 index, bool is_sub_menu)
{
	// const u32 menu_block_height = curr_menu->n_entries + 5; // +5 for title and button instructions
	int menu_x = (menu_con->consoleWidth - MENU_WIDTH) >> 1;
	// int menu_y = MENU_DISP_Y + ((menu_con->consoleHeight - menu_block_height) >> 1);
	int menu_y = MENU_OFFSET_TITLE;
	
	// select menu console
	consoleSelect(menu_con);
	consoleClear();
	
	// menu title
	consoleSetCursor(menu_con, menu_x, menu_y++);
	ee_printf(curr_menu->name);
	menu_y++;
	
	// first separator
	// consoleSetCursor(menu_con, menu_x, menu_y++);
	// ee_printf("%*.*s", MENU_WIDTH, MENU_WIDTH, "============================================================");
	
	// menu entries
	for (u32 i = 0; i < curr_menu->n_entries; i++)
	{
		MenuEntry* entry = &(curr_menu->entries[i]);
		char* name = entry->name;
		bool is_selected = (i == index);
		
		consoleSetCursor(menu_con, menu_x, menu_y++);
		ee_printf(is_selected ? "\x1b[47;30m%2lu.%-*.*s\x1b[0m" : "%2lu.%-*.*s", i+1, MENU_WIDTH-3, MENU_WIDTH-3, name);
	}
	
	// button instructions
	menu_y = MENU_OFFSET_BUTTONS;
	consoleSetCursor(menu_con, menu_x, menu_y++);
	ee_printf("%-*.*s", MENU_WIDTH, MENU_WIDTH, "[A]:Choose [B]:Back");
	
	// second separator
	// consoleSetCursor(menu_con, menu_x, menu_y++);
	// ee_printf("%*.*s", MENU_WIDTH, MENU_WIDTH, "============================================================");
	
	
	// button descriptions (A/B)
	/*consoleSetCursor(menu_con, menu_x, menu_y++);
	ee_printf(is_sub_menu ? "A: Choose  B: Return" : "A: Choose");
	
	// button descriptions (START)
	consoleSetCursor(menu_con, menu_x, menu_y++);
	ee_printf("POWER for poweroff");*/
	
	
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
	u32 result = MENU_EXIT_REBOOT;
	
	// main menu processing loop
	while (true) {
		// update menu and description (on demand)
		if ((index != last_index) || (curr_menu != last_menu)) {
			menuDraw(curr_menu, menu_con, index, menu_lvl);
			menuShowDesc(curr_menu, desc_con, index);
			last_index = index;
			last_menu = curr_menu;

			GX_textureCopy((u64*)RENDERBUF_TOP, (240 * 2)>>4, (u64*)GFX_getFramebuffer(SCREEN_TOP),
						   (240 * 2)>>4, SCREEN_SIZE_TOP + SCREEN_SIZE_SUB);
			GFX_swapFramebufs();
		}
		GFX_waitForEvent(GFX_EVENT_PDC0, true); // VBlank
		
		if(hidGetPowerButton(true)) // handle power button
		{
			power_off();
		}
		
		hidScanInput();
		const u32 kDown = hidKeysDown();
		const u32 kHeld = hidKeysHeld();
		
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
	}

	return result;
}
