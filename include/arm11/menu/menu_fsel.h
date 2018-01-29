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


#define BRWS_MAX_ENTRIES	13
#define BRWS_OFFSET_TITLE	 3
#define BRWS_OFFSET_BUTTONS	19
#define BRWS_WIDTH			40



#ifdef __cplusplus
extern "C"
{
#endif

bool menuFileSelector(char* res_path, PrintConsole* menu_con, const char* start, const char* pattern, bool allow_root, bool select_dirs);

#ifdef __cplusplus
}
#endif
