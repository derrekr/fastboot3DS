#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "types.h"
#include "util.h"
#include "hid.h"
#include "arm9/timer.h"
#include "arm9/main.h"
#include "arm9/ui.h"
#include "arm9/console.h"
#include "banner_ppm_bin.h"


static u8 randomColor;
static bool verbose = false;

static const void *bannerData = banner_ppm_bin;



static void uiDrawPPM(unsigned start_x, unsigned start_y, const u8 *data);

static void consoleMainInit()
{
	/* Initialize console for both screens using the two different PrintConsole we have defined */
	consoleInit(SCREEN_TOP, &con_top);
	consoleSetWindow(&con_top, 1, 1, con_top.windowWidth - 2, con_top.windowHeight - 2);
	
	// randomize color
	randomColor = (rng_get_byte() % 6) + 1;
	drawConsoleWindow(&con_top, 2, randomColor);
	
	consoleInit(SCREEN_LOW, &con_bottom);
	
	consoleSelect(&con_top);
}

void uiDrawSplashScreen()
{
	uiDrawPPM(30,30, bannerData);
	wait(0x6000000);	// test
}

void uiInit()
{
	consoleMainInit();
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

void uiSetVerbose(bool verb)
{
	verbose = verb;
}

/* TODO: Should look similar to uiShowMessageWindow */
bool uiDialogYesNo(const char *text, const char *textYes, const char *textNo)
{
	u32 keys;
	
	/* Print dialog */
	uiPrintInfo("\n%s\n", text);
	uiPrintInfo("\t(A): %s\n\t(B): %s\n", textYes, textNo);
	
	/* Get user input */
	do {
		hidScanInput();
		keys = hidKeysDown() & HID_KEY_MASK_ALL;
		if(keys == KEY_A)
			return true;
		if(keys & KEY_B)
			return false;
	} while(1);
}

void uiPrintIfVerbose(const char *const format, ...)
{
	if(verbose)
	{
		const unsigned int width = (unsigned int)consoleGet()->consoleWidth;
		char tmp[width + 1];

		va_list args;
		va_start(args, format);
		vsnprintf(tmp, width + 1, format, args);
		va_end(args);

		size_t len = strlen(tmp);
		printf("\x1B[0m%*s%s\n", (width - len) / 2, "", tmp);
	}
}

/* Prints a given text in the center of the current line */
void uiPrintCentered(const char *const format, unsigned int color, ...)
{
	const unsigned int width = (unsigned int)consoleGet()->consoleWidth;
	char tmp[width + 1];

	va_list args;
	va_start(args, color);
	vsnprintf(tmp, width + 1, format, args);
	va_end(args);

	size_t len = strlen(tmp);
	printf("\x1B[%um%*s%s\n", color, (width - len) / 2, "", tmp);
}

void uiPrintCenteredInLine(unsigned int y, const char *const format, ...)
{
	const unsigned int width = (unsigned int)consoleGet()->consoleWidth;
	char tmp[width + 1];

	va_list args;
	va_start(args, format);
	vsnprintf(tmp, width + 1, format, args);
	va_end(args);

	size_t len = strlen(tmp);
	printf("\x1b[%u;%uH", y, consoleGet()->cursorX);
	printf("%*s%s\n", (width - len) / 2, "", tmp);
}

/* Prints a given text at a certain position in the current window */
void uiPrintTextAt(unsigned int x, unsigned int y, const char *const format, ...)
{
	char tmp[256];

	va_list args;
	va_start(args, format);
	vsnprintf(tmp, 256, format, args);
	va_end(args);

	printf("\x1b[%u;%uH%s", y, x, tmp);
}

/* Prints a given text surrounded by a graphical window */
/* centered in the middle of the screen. */
/* Waits for the user to press any button, after that the */
/* original framebuffer gets restored */
/*void uiShowMessageWindow(format, args...)
{
	// TODO
}*/


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


static void uiDrawPPM(unsigned start_x, unsigned start_y, const u8 *data)
{
	unsigned width, height;
	u16 *framebuf = (u16 *) FRAMEBUF_TOP_A_1;
	
	/* get image dimensions */
	const char *ptr = (const char *) data + 3;
		while(*ptr != 0x0A) ptr++;
	ptr++;
	
	//sscanf(ptr, "%i %i", &width, &height);
	width = 204; height = 21;
	
	const u8 *imagedata = data + 0x26;	// skip ppm header
	
	
	for(unsigned x = 0; x < width; x++)
	{
		for(unsigned y = start_y; y < height+start_y; y++)
		{
			framebuf = &((u16*)FRAMEBUF_TOP_A_1)[SCREEN_HEIGHT_TOP * x + y];
			u8 *pixeldata = &imagedata[(y*width+x)*3];
			*framebuf = RGB8_to_565(pixeldata[0], pixeldata[1], pixeldata[2]);
		}
	}
}
