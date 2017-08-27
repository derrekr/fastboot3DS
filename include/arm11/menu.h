#pragma once

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
#include "arm11/console.h"

#define MENU_MAX_ENTRIES	12
#define MENU_MAX_DEPTH		4

#define MENU_DISP_Y			-2
#define MENU_WIDTH			28
#define WORDWRAP_WIDTH		58
	
#define MENU_EXIT_REBOOT	0
#define MENU_EXIT_POWEROFF	1

/**
 * @brief Menu entry description struct, used by MenuInfo.
 */
typedef struct {
    char* name;					///< Displayed name of the menu entry.
    char* desc;					///< Description for menu entry.
    u32 (*function)(PrintConsole *con, u32 param);	///< Function called by menu entry.
    u32 param;					///< Paramater for menu entry function / if function == NULL, offset to is_sub_menu.
} MenuEntry;

/**
 * @brief Menu info description struct, used in an array to represent the menu.
 */
typedef struct {
    char* name;								///< Displayed name on top of the menu.
    u32 n_entries;							///< Number of entries in the menu.
    MenuEntry entries[MENU_MAX_ENTRIES];	///< An array of menu entries.
} MenuInfo;

u32 menuProcess(PrintConsole* menu_con, PrintConsole* desc_con, MenuInfo* info);
