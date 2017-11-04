#pragma once

/*
 *   This file is part of fastboot 3DS
 *   Copyright (C) 2017 derrek, profi200
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


#define FLAG_ROTATED     (1u<<3)
#define FLAG_COMPRESSED  (1u<<4)
#define FLAG_SWAPPED     (1u<<5)


enum
{
	FORMAT_RGB565  = 0,
	FORMAT_RGB8    = 1,
	FORMAT_RGBA8   = 2,
	FORMAT_INVALID = 7
};

typedef struct
{
	u32 magic;
	u16 width;
	u16 height;
	u32 flags;
} SplashHeader;



void getSplashDimensions(const void *const data, u32 *const width, u32 *const height);
bool drawSplashscreen(const void *const data, s32 startX, s32 startY);
