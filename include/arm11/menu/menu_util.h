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


char* mallocpyString(const char* str);
void truncateString(char* dest, const char* orig, int nsize, int tpos);
void formatBytes(char* str, u64 bytes);
void keysToString(u32 keys, char* string);
void stringWordWrap(char* str, int llen);

u32 stringGetHeight(const char* str);
u32 stringGetWidth(const char* str);

u32 ee_printf_line_center(const char *const fmt, ...);
u32 ee_printf_screen_center(const char *const fmt, ...);

void updateScreens(void);
void outputEndWait(void);
void sleepmode(void);
