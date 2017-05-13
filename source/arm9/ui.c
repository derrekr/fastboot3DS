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


static void consoleMainInit()
{
	/* Initialize console for both screens using the two different PrintConsole we have defined */
	consoleInit(SCREEN_TOP, &con_top);
	con_top.windowX = 1;
	con_top.windowY = 1;
	con_top.windowWidth -= 2;
	con_top.windowHeight -= 2;
	
	drawConsoleWindow(&con_top, 4, con_top.fg);
	
	consoleInit(SCREEN_LOW, &con_bottom);
	
	consoleSelect(&con_top);
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

/* Prints a given text in the center of the current line */
void uiPrintCentered(format, args...)
{
	// TODO
}

/* Prints a given text at a certain position in the current window */
void uiPrintTextAt(unsigned x, unsigned y, format, args...)
{
	// TODO
}

/* Prints a given text surrounded by a graphical window */
/* centered in the middle of the screen. */
/* Waits for the user to press any button, after that the */
/* original framebuffer gets restored */
void uiShowMessageWindow(format, args...)
{
	// TODO
}


void uiPrintProgressBar(unsigned x, unsigned y, unsigned width, unsigned height,
							double percentage)
{
	// TODO
}

