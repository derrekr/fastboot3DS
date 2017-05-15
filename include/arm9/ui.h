#pragma once

#include "arm9/console.h"
#include "hid.h"

#define uiPrintInfo(format, ...)    uiPrint(format, 0, false, ##__VA_ARGS__)
#define uiPrintWarning(format, ...) uiPrint(format, 33, false, ##__VA_ARGS__)
#define uiPrintError(format, ...)   uiPrint(format, 31, false, ##__VA_ARGS__)


// PrintConsole for each screen
PrintConsole con_top, con_bottom;

void uiInit();
void uiDrawSplashScreen();
void uiClearConsoles();
void clearConsole(int which);
void uiSetVerbose(bool verb);
bool uiDialogYesNo(const char *text, const char *textYes, const char *textNo);
void uiPrintIfVerbose(const char *const format, ...);
void uiPrint(const char *const format, unsigned int color, bool centered, ...);
void uiPrintCenteredInLine(unsigned int y, const char *const format, ...);
void uiPrintTextAt(unsigned int x, unsigned int y, const char *const format, ...);
//void uiShowMessageWindow(format, args...);
void uiPrintProgressBar(unsigned int x, unsigned int y, unsigned int w,
                        unsigned int h, unsigned int cur, unsigned int max);
