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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "arm9/splash.h"
#include "arm9/fmt.h"
#include "gfx.h"



void lz11Decompress(const void *in, void *out, u32 size);

static bool validateSplashHeader(SplashHeader *header)
{
	if(memcmp(&header->magic, "SPLA", 4)) return false;

	const u16 width = header->width, height = header->height;
	if(width > SCREEN_WIDTH_TOP || height > SCREEN_HEIGHT_TOP) return false;

	const u32 flags = header->flags;
	if((flags & FORMAT_INVALID) != FORMAT_RGB565) return false;
	if(!(flags & FLAG_ROTATED) || flags & FLAG_SWAPPED) return false;

	return true;
}

bool drawSplashscreen(const void *const data)
{
	SplashHeader header;
	memcpy(&header, data, sizeof(SplashHeader));
	if(!validateSplashHeader(&header)) return false;

	const u16 width = header.width, height = header.height;
	const u32 flags = header.flags;
	const bool isCompressed = flags & FLAG_COMPRESSED;

	u16 *imgData;
	if(isCompressed)
	{
		imgData = (u16*)malloc(width * height * 2);
		if(!imgData) return false;
		lz11Decompress(data + sizeof(SplashHeader), imgData, width * height * 2);
	}
	else imgData = (u16*)(data + sizeof(SplashHeader));

	u16 *fb = (u16*)FRAMEBUF_TOP_A_1;
	const u32 yy = (SCREEN_HEIGHT_TOP - height) / 2;
	const u32 xx = (SCREEN_WIDTH_TOP - width) / 2;
	for(u32 x = 0; x < width; x++)
	{
		for(u32 y = 0; y < height; y++)
		{
			fb[(x + xx) * SCREEN_HEIGHT_TOP + y + yy] = imgData[x * height + y];
		}
	}

	if(isCompressed) free(imgData);

	return true;
}
