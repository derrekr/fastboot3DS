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
#include "arm11/menu/splash.h"
#include "arm11/lz11.h"
#include "hardware/gfx.h"



void getSplashDimensions(const void *const data, u32 *const width, u32 *const height)
{
	const SplashHeader *const header = (const SplashHeader *const)data;

	if(width) *width = header->width;
	if(height) *height = header->height;
}

static bool validateSplashHeader(const SplashHeader *const header)
{
	if(memcmp(&header->magic, "SPLA", 4)) return false;

	const u32 width = header->width, height = header->height;
	if(width == 0 || width > SCREEN_WIDTH_TOP || height == 0 || height > SCREEN_HEIGHT_TOP)
		return false;

	const u32 flags = header->flags;
	if((flags & FORMAT_INVALID) != FORMAT_RGB565) return false;
	if(!(flags & FLAG_ROTATED) || flags & FLAG_SWAPPED) return false;

	return true;
}

bool drawSplashscreen(const void *const data, s32 startX, s32 startY)
{
	const SplashHeader *const header = (const SplashHeader *const)data;
	if(!validateSplashHeader(header)) return false;

	const u32 width = header->width, height = header->height;
	const u32 flags = header->flags;
	const bool isCompressed = flags & FLAG_COMPRESSED;

	u16 *imgData;
	if(isCompressed)
	{
		imgData = (u16*)malloc(width * height * 2);
		if(!imgData) return false;
		lz11Decompress(data + sizeof(SplashHeader), imgData, width * height * 2);
	}
	else imgData = (u16*)(data + sizeof(SplashHeader));

	u32 xx, yy;
	if(startX < 0 || (u32)startX > SCREEN_WIDTH_TOP - width) xx = (SCREEN_WIDTH_TOP - width) / 2;
	else xx = (u32)startX;
	if(startY < 0 || (u32)startY > SCREEN_HEIGHT_TOP - height) yy = (SCREEN_HEIGHT_TOP - height) / 2;
	else yy = (u32)startY;

	u16 *fb = (u16*)RENDERBUF_TOP;
	for(u32 x = 0; x < width; x++)
	{
		for(u32 y = 0; y < height; y++)
		{
			fb[(x + xx) * SCREEN_HEIGHT_TOP + y + yy] = imgData[x * height + y];
		}
	}

	if(isCompressed) free(imgData);

	GX_textureCopy((u64*)RENDERBUF_TOP, 0, (u64*)GFX_getFramebuffer(SCREEN_TOP), 0, SCREEN_SIZE_TOP);
	GFX_swapFramebufs();
	GFX_waitForEvent(GFX_EVENT_PDC0, true); // VBlank

	return true;
}
