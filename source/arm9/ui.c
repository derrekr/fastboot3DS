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
#include <stdarg.h>
#include "types.h"
#include "arm9/fmt.h"
#include "util.h"
#include "arm9/hid.h"
#include "pxi.h"
#include "arm9/timer.h"
#include "arm9/main.h"
#include "arm9/ui.h"
#include "arm9/console.h"
#include "banner_ppm.h"
#include "bootwarning_ppm.h"
#include "bootfail_ppm.h"


static u8 randomColor;
static bool verbose = false;

static const void *bannerData = banner_ppm;
static const void *bootWarningData = bootwarning_ppm;
static const void *bootFailureData = bootfail_ppm;


static void uiGetPPMInfo(const u8 *data, unsigned *width, unsigned *height);
static void uiDrawPPM(unsigned start_x, unsigned start_y, const u8 *data);

static void consoleMainInit()
{
	/* Initialize console for both screens using the two different PrintConsole we have defined */
	consoleInit(SCREEN_TOP, &con_top, true);
	consoleSetWindow(&con_top, 1, 1, con_top.windowWidth - 2, con_top.windowHeight - 2);
	
	// randomize color
	randomColor = (rng_get_byte() % 6) + 1;
	
	consoleInit(SCREEN_LOW, &con_bottom, true);
	
	consoleSelect(&con_top);
}

void uiDrawSplashScreen()
{
	unsigned width, height;
	
	uiGetPPMInfo(bannerData, &width, &height);
	// centered draw
	uiDrawPPM((SCREEN_WIDTH_TOP - width) / 2, (SCREEN_HEIGHT_TOP - height) / 2, bannerData);
}

void uiInit()
{
	consoleMainInit();
}

void uiDrawConsoleWindow()
{
	drawConsoleWindow(&con_top, 2, randomColor);
}

static void clearConsoles()
{
	consoleSelect(&con_bottom);
	consoleClear();
	consoleSelect(&con_top);
	consoleClear();
}

void uiClearConsoles()
{
	clearConsoles();
}

void clearConsole(int which)
{
	if(which)
		consoleSelect(&con_top);
	else
		consoleSelect(&con_bottom);
		
	consoleClear();
}

void uiSetVerboseMode(bool verb)
{
	verbose = verb;
}

bool uiGetVerboseMode()
{
	return verbose;
}

/* TODO: Should look similar to uiShowMessageWindow */
bool uiDialogYesNo(int screen, const char *textYes, const char *textNo, const char *const format, ...)
{
	char tmp[256];
	char lastline[256];
	u32 keys;

	va_list args;
	va_start(args, format);
	ee_vsnprintf(tmp, 256, format, args);
	va_end(args);
	
	ee_snprintf(lastline, 256, "(A): %s  (B): %s", textYes, textNo);
	
	/* Print dialog */
	keys = uiDialog(tmp, lastline, KEY_A | KEY_B, screen, 0, 0, true);
	
	/* Evaluate user input */
	if(keys == KEY_A)
		return true;
	
	// KEY_B
	return false;
}

void uiPrintIfVerbose(const char *const format, ...)
{
	if(verbose)
	{
		char tmp[256];

		va_list args;
		va_start(args, format);
		ee_vsnprintf(tmp, 256, format, args);
		va_end(args);
		
		ee_printf(tmp);
	}
}

/* Prints a given text at the current position */
void uiPrint(const char *const format, unsigned int color, bool centered, ...)
{
	const unsigned int width = (unsigned int)consoleGet()->consoleWidth;
	char tmp[256];

	va_list args;
	va_start(args, centered);
	ee_vsnprintf(tmp, 556, format, args);
	va_end(args);

	if(centered)
	{
		// Warning. The string must be <= the console width here!
		size_t len = strlen(tmp);
		ee_printf("\x1B[%um%*s%s\x1B[0m", color, (width - len) / 2, "", tmp);
	}
	else
	{
		ee_printf("\x1B[%um%s\x1B[0m", color, tmp);
	}
}

void uiPrintCenteredInLine(unsigned int y, const char *const format, ...)
{
	const unsigned int width = (unsigned int)consoleGet()->consoleWidth;
	char tmp[width + 1];

	va_list args;
	va_start(args, format);
	ee_vsnprintf(tmp, width + 1, format, args);
	va_end(args);

	size_t len = strlen(tmp);
	ee_printf("\x1b[%u;%uH", y, 0);
	ee_printf("%*s%s\n", (width - len) / 2, "", tmp);
}

/* Prints a given text at a certain position in the current window */
void uiPrintTextAt(unsigned int x, unsigned int y, const char *const format, ...)
{
	char tmp[256];

	va_list args;
	va_start(args, format);
	ee_vsnprintf(tmp, 256, format, args);
	va_end(args);

	ee_printf("\x1b[%u;%uH%s", y, x, tmp);
}

/* Prints a given text surrounded by a graphical window */
/* centered in the middle of the screen. */
/* Waits for the user to press any button, after that the */
/* original framebuffer gets restored */
u32 uiDialog(const char *const format, const char *const lastLine, u32 waitKeys,
                    int screen, unsigned int x, unsigned int y, bool centered, ...)
{
	char tmp[256];

	va_list args;
	va_start(args, centered);
	ee_vsnprintf(tmp, 256, format, args);
	va_end(args);

	char *ptr = tmp;
	unsigned int lines = lastLine ? 5 : 3, longestLine = 2, curLen = 2;
	while(*ptr)
	{
		switch(*ptr)
		{
			case '\n':
				lines++;
				curLen = 2;
				break;
			default:
				curLen++;
		}
		if(curLen > longestLine) longestLine = curLen;

		ptr++;
	}
	
	if(lastLine)
	{
		unsigned int lastLineLen = strlen(lastLine) + 2;
		if(lastLineLen > longestLine) longestLine = lastLineLen;
	}

	PrintConsole *prevCon = consoleGet();
	PrintConsole windowCon;
	consoleInit(screen, &windowCon, false);

	if(centered)
	{
		x = (windowCon.windowWidth / 2) - (longestLine / 2);
		y = (windowCon.windowHeight / 2) - (lines / 2);
	}
	consoleSetWindow(&windowCon, x, 30 - y - lines, longestLine, lines);

	u8 *fbBackup = (u8*)malloc(screen ? SCREEN_SIZE_TOP : SCREEN_SIZE_SUB);
	u32 keys = 0;
	if(fbBackup)
	{
		u16 *fb = consoleGet()->frameBuffer;

		// TODO: Optimize to only backup what will be overwritten. I'm lazy.
		memcpy(fbBackup, fb, screen ? SCREEN_SIZE_TOP : SCREEN_SIZE_SUB);

		ee_printf("\x1B[37m\x1B[40m\x1B[2J\n");

		const char *linePtr = tmp;
		while(1)
		{
			unsigned int length = 0;
			while(linePtr[length] != '\n' && linePtr[length] != '\0') length++;
			ee_printf(" %.*s\n", length, linePtr);
			if(*(linePtr + length) == '\0') break;
			linePtr += length + 1;
		}
		
		if(lastLine)
			ee_printf("\x1b[%u;%uH%s", lines - 2, (longestLine - strlen(lastLine)) / 2, lastLine);

		const u16 color = consoleGetRGB565Color(randomColor);
		for(u32 xx = x * 8 + 1; xx < x * 8 + (longestLine * 8) - 1; xx++)
		{
			fb[xx * SCREEN_HEIGHT_SUB + (y * 8)] = color;
			fb[xx * SCREEN_HEIGHT_SUB + (y * 8) + (lines * 8) - 1] = color;
		}
		for(u32 xx = x * 8 + 3; xx < x * 8 + (longestLine * 8) - 3; xx++)
		{
			fb[xx * SCREEN_HEIGHT_SUB + (y * 8) + 2] = color;
			fb[xx * SCREEN_HEIGHT_SUB + (y * 8) + (lines * 8) - 3] = color;
		}
		for(u32 yy = y * 8; yy < y * 8 + (lines * 8); yy++)
		{
			fb[x * 8 * SCREEN_HEIGHT_SUB + yy] = color;
			fb[(x * 8 + (longestLine * 8) - 1) * SCREEN_HEIGHT_SUB + yy] = color;
		}
		for(u32 yy = y * 8 + 2; yy < y * 8 + (lines * 8) - 2; yy++)
		{
			fb[(x * 8 + 2) * SCREEN_HEIGHT_SUB + yy] = color;
			fb[(x * 8 + (longestLine * 8) - 3) * SCREEN_HEIGHT_SUB + yy] = color;
		}

		if(waitKeys)
		{
			do
			{
				hidScanInput();
			} while(!(keys = hidKeysDown() & waitKeys));
		}

		memcpy(fb, fbBackup, screen ? SCREEN_SIZE_TOP : SCREEN_SIZE_SUB);
	}
	free(fbBackup); // free() checks for NULL

	consoleSelect(prevCon);

	return keys;
}

void uiPrintDevModeRequirement()
{
	// This should use uiShowMessageWindow some day...
	uiPrintError("This feature is blocked!");
	uiPrintError("You must enable Developer Mode to use it.");
	
	do {
		hidScanInput();
		u32 keys = hidKeysDown() & HID_KEY_MASK_ALL;
		if(keys == KEY_A)
			break;
	} while(1);
}

void uiPrintProgressBar(unsigned int x, unsigned int y, unsigned int w,
                        unsigned int h, unsigned int cur, unsigned int max)
{
	u16 *fb = consoleGet()->frameBuffer;
	const u16 color = consoleGetFgColor();


	for(u32 xx = x + 1; xx < x + w - 1; xx++)
	{
		fb[xx * SCREEN_HEIGHT_TOP + y] = color;
		fb[xx * SCREEN_HEIGHT_TOP + y + h - 1] = color;
	}

	for(u32 yy = y; yy < y + h; yy++)
	{
		fb[x * SCREEN_HEIGHT_TOP + yy] = color;
		fb[(x + w - 1) * SCREEN_HEIGHT_TOP + yy] = color;
	}

	for(u32 xx = x + 2; xx < (u32)(((float)(x + w - 2) / max) * cur); xx++)
	{
		for(u32 yy = y + 2; yy < y + h - 2; yy++)
		{
			fb[xx * SCREEN_HEIGHT_TOP + yy] = color;
		}
	}
}

static inline void drawPixel(unsigned x, unsigned y, u16 color)
{
	u16 *framebuf = (u16 *) FRAMEBUF_TOP_A_1;
	framebuf[x * SCREEN_HEIGHT_TOP + y] = color;
}

static void uiGetPPMInfo(const u8 *data, unsigned *width, unsigned *height)
{
	/* get image dimensions */
	const char *ptr = (const char *) data + 3;
		while(*ptr != 0x0A) ptr++;
	ptr++;
	
	mysscanf(ptr, "%u %u", width, height);
}

static void uiDrawPPM(unsigned start_x, unsigned start_y, const u8 *data)
{
	unsigned width, height;
	
	/* get image dimensions */
	uiGetPPMInfo(data, &width, &height);
	
	const u8 *imagedata = data + 0x26;	// skip ppm header
	
	u16 color;
	
	for(unsigned x = 0; x < width; x++)
	{
		for(unsigned y = height; y > 0; y--)
		{
			const u8 *pixeldata = &imagedata[(x + (y-1) * width)*3];
			color = RGB8_to_565(pixeldata[2], pixeldata[0], pixeldata[1]);
			drawPixel(start_x + x, start_y + height - y, color);
		}
	}
}

static void clearPPM(unsigned start_x, unsigned start_y, const u8 *data)
{
	unsigned width, height;
	
	/* get image dimensions */
	uiGetPPMInfo(data, &width, &height);
	
	u16 color = 0;
	
	for(unsigned x = 0; x < width; x++)
	{
		for(unsigned y = height; y > 0; y--)
		{
			drawPixel(start_x + x, start_y + height - y, color);
		}
	}
}

void uiPrintBootWarning()
{
	unsigned width, height;
	
	uiGetPPMInfo(bootWarningData, &width, &height);
	uiDrawPPM((SCREEN_WIDTH_TOP - width) / 2, SCREEN_HEIGHT_TOP + 10, bootWarningData);
	TIMER_sleep(400);
}

void uiPrintBootFailure()
{
	unsigned width, height;
	
	uiGetPPMInfo(bootWarningData, &width, &height);
	clearPPM((SCREEN_WIDTH_TOP - width) / 2, SCREEN_HEIGHT_TOP + 10, bootWarningData);
	uiGetPPMInfo(bootFailureData, &width, &height);
	uiDrawPPM((SCREEN_WIDTH_TOP - width) / 2, SCREEN_HEIGHT_TOP + 10, bootFailureData);
	TIMER_sleep(400);
}

bool uiCheckHomePressed(u32 msTimeout)
{
	u32 curMs;
	bool successFlag;
	
	/* Check PXI Response register */
	u32 replyCode = PXI_tryRecvWord(&successFlag);
	
	curMs = 0;
	
	do {
		
		while(successFlag)
		{
			if(replyCode == PXI_RPL_HOME_PRESSED ||
				replyCode == PXI_RPL_HOME_HELD)
			{
				return true;
			}
			// maybe there's more..?
			replyCode = PXI_tryRecvWord(&successFlag);
		}

		TIMER_sleep(1);
		curMs++;

	} while(curMs < msTimeout);
	
	/* timed out ... */
	
	return false;
}
