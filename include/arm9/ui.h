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
void uiClearConsoles();
void clearConsole(int which);
bool uiDialogYesNo(const char *text, const char *textYes, const char *textNo);
