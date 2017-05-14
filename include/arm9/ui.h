#pragma once

#include "arm9/console.h"
#include "hid.h"

#define uiPrintInfo(format, args...)	       	\
					printf("\e[0m");			\
					printf(format, ## args);	\

#define uiPrintWarning(format, args...)	       	\
					printf("\x1B[33m");			\
					printf(format, ## args);	\

#define uiPrintError(format, args...)	       	\
					printf("\x1B[31m");			\
					printf(format, ## args);	\


// PrintConsole for each screen
PrintConsole con_top, con_bottom;

void uiInit();
void uiDrawSplashScreen();
void uiClearConsoles();
void clearConsole(int which);
bool uiDialogYesNo(const char *text, const char *textYes, const char *textNo);
void uiPrintCentered(const char *const format, ...);
void uiPrintTextAt(unsigned int x, unsigned int y, const char *const format, ...);
//void uiShowMessageWindow(format, args...);
void uiPrintProgressBar(unsigned int x, unsigned int y, unsigned int width,
                        unsigned int height, float percentage);
