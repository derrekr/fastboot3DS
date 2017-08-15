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

#include "arm9/console.h"
#include "arm9/hardware/hid.h"


#define uiPrintInfo(format, ...)    uiPrint(format, 0, false, ##__VA_ARGS__)
#define uiPrintWarning(format, ...) uiPrint(format, 33, false, ##__VA_ARGS__)
#define uiPrintError(format, ...)   uiPrint(format, 31, false, ##__VA_ARGS__)


// PrintConsole for each screen
PrintConsole con_top, con_bottom;



void uiInit();
void uiDrawSplashScreen();
void uiDrawConsoleWindow();
void uiClearConsoles();
void clearConsole(int which);
void uiSetVerboseMode(bool verb);
bool uiGetVerboseMode();
bool uiDialogYesNo(int screen, const char *textYes, const char *textNo, const char *const format, ...);
void uiPrintIfVerbose(const char *const format, ...);
void uiPrint(const char *const format, unsigned int color, bool centered, ...);
void uiPrintCenteredInLine(unsigned int y, const char *const format, ...);
void uiPrintTextAt(unsigned int x, unsigned int y, const char *const format, ...);
u32 uiDialog(const char *const format, const char *const lastLine, u32 waitKeys,
                        int screen, unsigned int x, unsigned int y, bool centered, ...);
void uiPrintProgressBar(unsigned int x, unsigned int y, unsigned int w,
                        unsigned int h, unsigned int cur, unsigned int max);
void uiPrintDevModeRequirement();
void uiPrintBootWarning();
void uiPrintBootFailure();
bool uiCheckHomePressed(u32 msTimeout);
void uiWaitForAnyPadkey();
