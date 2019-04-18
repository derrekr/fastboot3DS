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

#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "fs.h"
#include "hardware/gfx.h"
#include "arm11/hardware/hid.h"
#include "arm11/hardware/mcu.h"
#include "arm.h"
#include "arm11/menu/menu_color.h"
#include "arm11/menu/menu_util.h"
#include "arm11/menu/splash.h"
#include "arm11/console.h"
#include "arm11/fmt.h"

#define BORDER_WIDTH	2 // in pixel


void truncateString(char* dest, const char* orig, int nsize, int tpos)
{
	int osize = strlen(orig);
	
	if (nsize < 0)
	{
		return;
	} else if (nsize <= 3)
	{
		ee_snprintf(dest, nsize, orig);
	} else if (nsize >= osize)
	{
		ee_snprintf(dest, nsize + 1, orig);
	} else
	{
		if (tpos + 3 > nsize) tpos = nsize - 3;
		ee_snprintf(dest, nsize + 1, "%-.*s...%-.*s", tpos, orig, nsize - (3 + tpos), orig + osize - (nsize - (3 + tpos)));
	}
}

void formatBytes(char* str, u64 bytes)
{
	// str should be 32 byte in size, just to be safe
	const char* units[] = {"  Byte", " kiB", " MiB", " GiB"};
	
	if (bytes < 1024)
	{
		ee_snprintf(str, 32, "%llu%s", bytes, units[0]);
	}
	else
	{
		u32 scale = 1;
		u64 bytes100 = (bytes * 100) >> 10;
		for(; (bytes100 >= 1024*100) && (scale < 3); scale++, bytes100 >>= 10);
		ee_snprintf(str, 32, "%llu.%llu%s", bytes100 / 100, (bytes100 % 100) / 10, units[scale]);
	}
}

static const char * convTable[] = {
	"A", "B", "SELECT", "START", "RIGHT", "LEFT",
	"UP", "DOWN", "R", "L", "X", "Y"
};

void keysToString(u32 keys, char* string)
{
	char* ptr = string;
	bool first = true;
	for (u32 i = 0; i < 12; i++)
	{
		if (keys & (1<<i))
		{
			ptr += ee_sprintf(ptr, "%s[%s]", first ? "" : "+", convTable[i]);
			first = false;
		}
	}
	if (first) // backup solution for no buttons
		ee_sprintf(ptr, "(no buttons)");
}

u32 stringGetHeight(const char* str)
{
	u32 height = 1;
	for (char* lf = strchr(str, '\n'); (lf != NULL); lf = strchr(lf + 1, '\n'))
		height++;
	return height;
}

u32 stringGetWidth(const char* str)
{
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

void stringWordWrap(char* str, int llen)
{
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

u32 ee_printf_line_center(const char *const fmt, ...)
{
	char buf[64];
	va_list args;
	va_start(args, fmt);
	ee_vsnprintf(buf, 64, fmt, args);
	va_end(args);
	
	PrintConsole* con = consoleGet();
	int pad = (con->windowWidth - strlen(buf)) / 2;
	if (pad < 0) pad = 0;
	con->cursorX = 0;
	
	return ee_printf("%*.*s%s\n", pad, pad, "", buf);
}

u32 ee_printf_screen_center(const char *const fmt, ...)
{
	u32 res = 0;
	
	char buf[512];
	va_list args;
	va_start(args, fmt);
	ee_vsnprintf(buf, 512, fmt, args);
	va_end(args);
	
	PrintConsole *con = consoleGet();
	int x = (con->windowWidth - stringGetWidth(buf)) >> 1;
	int y = (con->windowHeight - stringGetHeight(buf)) >> 1;
	
	consoleClear();
	for (char* str = strtok(buf, "\n"); str != NULL; str = strtok(NULL, "\n"))
	{
		consoleSetCursor(con, x, y++);
		res += ee_printf(str);
	}
	
	return res;
}

// prints a progress indicator (no args allowed, for byte values)
u32 ee_printf_progress(const char *const fmt, u32 w, u64 curr, u64 max)
{
	u32 res = 0;
	
	u32 prog_w = (u32) ((max > 0) && (curr <= max)) ? ((u64) curr * w) / max : 0;
    u32 prog_p = (u32) ((max > 0) && (curr <= max)) ? ((u64) curr * 100) / max : 0;
	
	
	res += ee_printf(ESC_SCHEME_ACCENT1 "%s" ESC_RESET " | ", fmt);
	
	u32 i = 0;
	ee_printf(ESC_SCHEME_WEAK);
	for (; i < prog_w; i++) res += ee_printf("\xDB");
	for (; i < w; i++) res += ee_printf("\xB1");
	ee_printf(ESC_RESET);
	
	res += ee_printf(" | %s%lu%%%s | %s%llu / %llu MiB%s\r",
		(curr == max) ? ESC_SCHEME_GOOD : "", prog_p, (curr == max) ? ESC_RESET : "",
		(curr == max) ? ESC_SCHEME_GOOD : "", curr / 0x100000, max / 0x100000, (curr == max) ? ESC_RESET : "");
	
	
	return res;
}

void clearScreens(void)
{
	GX_memoryFill((u64*)RENDERBUF_TOP, 1u<<9, SCREEN_SIZE_TOP, 0, (u64*)RENDERBUF_SUB, 1u<<9, SCREEN_SIZE_SUB, 0);
	GFX_waitForEvent(GFX_EVENT_PSC0, true);
}

void drawTopBorder(void)
{
	static u16 color = 0;
	u16 *fb = (u16*)RENDERBUF_TOP;
	
	// get "random" color (from RTC seconds)
	if (!color)
	{
		u8 rtc[8];
		MCU_readRTC(rtc);
		color = consoleGetRGB565Color((rtc[0] % 6) + 1);
	}
	
	for(u32 x = 0; x < SCREEN_WIDTH_TOP; x++)
	{
		for(u32 y = 0; y < SCREEN_HEIGHT_TOP; y++)
		{
			if((x < BORDER_WIDTH) || (x >= (SCREEN_WIDTH_TOP - BORDER_WIDTH)) ||
			   (y < BORDER_WIDTH) || (y >= (SCREEN_HEIGHT_TOP - BORDER_WIDTH)))
			   *fb = color;
			fb++;
		}
	}
}

bool drawCustomSplash(const char* folder)
{
	// this assumes top and bottom screens cleared
	const u32 splash_max_size = sizeof(SplashHeader) + (SCREEN_WIDTH_TOP * SCREEN_HEIGHT_TOP * 2);
	char* splash_path =  (char*) malloc(FF_MAX_LFN + 1);
	u8* splash_buffer = (u8*) malloc(splash_max_size + (30 * 1024));
	bool res = false;
	s32 fHandle;
	
	if (!splash_path || !splash_buffer) goto fail;
	
	// try top splash
	ee_snprintf(splash_path, FF_MAX_LFN + 1, "%s/%s.spla", folder, CSPLASH_NAME_TOP);
	fHandle = fOpen(splash_path, FS_OPEN_EXISTING | FS_OPEN_READ);
	if (fHandle >= 0)
	{
		u32 splash_size = fSize(fHandle);
		u16 *const splash_data = (u16*)(splash_buffer + splash_max_size + (30 * 1024) - splash_size);
		if ((splash_size < sizeof(SplashHeader)) || (splash_size > splash_max_size) ||
			(fRead(fHandle, splash_data, splash_size) != FR_OK))
		{
			fClose(fHandle);
			goto fail;
		}
		fClose(fHandle);
		
		if (drawSplashscreen(splash_data, (u16*)splash_buffer, -1, -1, SCREEN_TOP))
			res = true;
	}
	
	// try bottom splash
	ee_snprintf(splash_path, FF_MAX_LFN + 1, "%s/%s.spla", folder, CSPLASH_NAME_SUB);
	fHandle = fOpen(splash_path, FS_OPEN_EXISTING | FS_OPEN_READ);
	if (fHandle >= 0)
	{
		u32 splash_size = fSize(fHandle);
		u16 *const splash_data = (u16*)(splash_buffer + splash_max_size + (30 * 1024) - splash_size);
		if ((splash_size < sizeof(SplashHeader)) || (splash_size > splash_max_size) ||
			(fRead(fHandle, splash_data, splash_size) != FR_OK))
		{
			fClose(fHandle);
			goto fail;
		}
		fClose(fHandle);
		
		if (drawSplashscreen(splash_data, (u16*)splash_buffer, -1, -1, SCREEN_SUB))
			res = true;
	}
	
	
	fail:
	
	if (splash_path) free(splash_path);
	if (splash_buffer) free(splash_buffer);
	
	return res;
}

// only intended to be ran when the shell is closed
void sleepmode(void)
{
	GFX_enterLowPowerState();
	MCU_setPowerLedState(PWLED_SLEEP);
	do
	{
		__wfi();
		hidScanInput();
	} while(!(hidKeysUp() & KEY_SHELL));
	MCU_setPowerLedState(PWLED_NORMAL);
	GFX_returnFromLowPowerState();
}

void updateScreens(void)
{
	GX_textureCopy((u64*)RENDERBUF_TOP, 0, (u64*)GFX_getFramebuffer(SCREEN_TOP),
				   0, SCREEN_SIZE_TOP + SCREEN_SIZE_SUB);
	GFX_swapFramebufs();
	GFX_waitForEvent(GFX_EVENT_PDC0, true); // VBlank
}

bool askConfirmation(const char *const fmt, ...)
{
	char buf[512];
	char* instr = buf;
	u32 kDown = 0, kHeld = 0;
	
	va_list args;
	va_start(args, fmt);
	instr += ee_vsnprintf(buf, 512, fmt, args);
	va_end(args);
	
	ee_snprintf(instr, 512 - (instr-buf), "\n \nPress [A] + [LEFT] to confirm\nPress [B] or [HOME] to cancel");
	ee_printf_screen_center(buf);
	updateScreens();
	
	do
	{
		GFX_waitForEvent(GFX_EVENT_PDC0, true);
		
		if(hidGetPowerButton(false)) // handle power button
			break;
		
		hidScanInput();
		kDown = hidKeysDown();
		kHeld = hidKeysHeld();
		
		if ((kHeld & KEY_A) && (kHeld & KEY_DLEFT)) return true;
		if (kDown & (KEY_SHELL)) sleepmode();
	}
	while (!(kDown & (KEY_B|KEY_HOME)));
	
	return false;
}

void outputEndWait(void)
{
	u32 kDown = 0;
	
	do
	{
		GFX_waitForEvent(GFX_EVENT_PDC0, true);
		
		if(hidGetPowerButton(false)) // handle power button
			break;
		
		hidScanInput();
		kDown = hidKeysDown();
		if (kDown & (KEY_SHELL)) sleepmode();
	}
	while (!(kDown & (KEY_B|KEY_HOME)));
}

bool userCancelHandler(bool cancelAllowed)
{
	hidScanInput();
	u32 kDown = hidKeysDown();
	u32 powerHeld = hidGetPowerButton(false);
	
	// detect force poweroff
	if (powerHeld & (1<<1))
		return true;
	
	if (kDown & KEY_SHELL)
	{
		sleepmode();
	}
	else if (kDown & (KEY_HOME|KEY_B) || powerHeld)
	{
		if (cancelAllowed)
		{
			ee_printf("\r%60.60s\r", ""); // delete current line
			ee_printf("Cancel current operation?\n(A to confirm, B to continue)\n\n");
			updateScreens();
			do
			{
				GFX_waitForEvent(GFX_EVENT_PDC0, true);
			
				hidScanInput();
				kDown = hidKeysDown();
				powerHeld = hidGetPowerButton(false);
				
				if (kDown & KEY_SHELL) sleepmode();
				if (kDown & KEY_A) return true;
				if (powerHeld & (1<<1)) return true;
			}
			while (!(kDown & KEY_B));
		}
		else
		{
			ee_printf("\r%60.60s\r", ""); // delete current line
			ee_printf("Cancel is not allowed here.\n\n");
		}
		
		hidGetPowerButton(true); // poweroff stopped
	}
	
	return false;
}
