#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "mem_map.h"
#include "arm9/console.h"
#include "arm9/dev.h"
#include "fatfs/ff.h"
#include "pxi.h"
#include "hid.h"
#include "util.h"
#include "arm9/main.h"
#include "arm9/timer.h"
#include "arm9/menu.h"
#include "arm9/nandimage.h"
#include "arm9/config.h"


#define MAX_SUBOPTIONS		3

static const char *optionStrings[] = {
	"Change Boot Mode",
	"Setup Boot Option",
	"Setup NAND Image"
};

static char optionSubStrings[MAX_SUBOPTIONS][0x20];

static unsigned int curOptionIndex;
static unsigned int curSubOptionIndex;
static unsigned int curHighlightedSubOptionIndex;
static bool			curSubOptionHighlighted;
static unsigned int curSubOptionCount;
static bool subOptionsActive;
static const unsigned int OptionCount = arrayEntries(optionStrings);
static int consoleX, consoleY;

void calcConsoleMetrics(void);
void rewindConsole(void);
void rewindConsoleX(void);
void handleBootMode(void);
void handleBootOption(void);
void handleNandImage(void);

bool menuOptions(void)
{
	u32 keys;
	
	PXI_sendWord(PXI_CMD_ALLOW_POWER_OFF);
	
	uiClearConsoles();
	consoleSelect(&con_bottom);
	
	calcConsoleMetrics();
	
	curOptionIndex = 0;
	curSubOptionIndex = 0;
	curHighlightedSubOptionIndex = 0;
	curSubOptionCount = 0;
	subOptionsActive = false;
	curSubOptionHighlighted = false;
	
	for(;;)
	{
		
		/* Draw screen */
		
		rewindConsole();
		
		for(unsigned int i=0; i<OptionCount + MAX_SUBOPTIONS; i++)
		{
			if(i<OptionCount)
			{
				printf("%s %s", i == curOptionIndex ? "*" : " ", optionStrings[i]);
				rewindConsoleX();
			
				if(subOptionsActive && i == curOptionIndex)
				{
					for(unsigned int j=0; j<curSubOptionCount; j++)
					{
						if(curSubOptionHighlighted && j == curHighlightedSubOptionIndex)
							printf(" %s  \x1B[33m%s\e[0m", j == curSubOptionIndex ? "*" : " ",
															optionSubStrings[j]);
						else
							printf(" %s  %s", j == curSubOptionIndex ? "*" : " ", optionSubStrings[j]);
						rewindConsoleX();
					}
				}
			}
			else
			{
				rewindConsoleX();
			}
		}
		
		
		/* Handle HID */
		
		hidScanInput();
		keys = hidKeysDown();
		
		if(keys & KEY_DUP || keys & KEY_DLEFT)
		{
			if(subOptionsActive)
			{
				if(curSubOptionIndex != 0)
					curSubOptionIndex --;
			}
			else
			{
				if(curOptionIndex != 0)
					curOptionIndex --;
			}
		}
		
		else if(keys & KEY_DDOWN || keys & KEY_DRIGHT)
		{
			if(subOptionsActive)
			{
				if(curSubOptionIndex < curSubOptionCount - 1)
					curSubOptionIndex ++;
			}
			else
			{
				if(curOptionIndex < OptionCount - 1)
					curOptionIndex ++;
			}
		}
		
		else if(keys & KEY_A)
		{
			switch(curOptionIndex)
			{
				case 0:	// "Change Boot Mode"
					handleBootMode();
					break;
				
				case 1: // "Setup Boot Option"
					handleBootOption();
					break;
				
				case 2: // "Setup NAND Image"
					handleNandImage();
					break;
				
				default:
					break;
			}
			
			if(!subOptionsActive)
			{
				subOptionsActive = true;
				curSubOptionIndex = 0;
			}
		}
		
		else if(keys & KEY_B)
		{
			if(subOptionsActive)
			{
				subOptionsActive = false;
				curSubOptionHighlighted = false;
				curSubOptionCount = 0;
			}
			else
			{
				goto done;
			}
		}
		
		switch(menuUpdateGlobalState())
		{
			case MENU_EVENT_HOME_PRESSED:
			case MENU_EVENT_POWER_PRESSED:
			case MENU_EVENT_SD_CARD_REMOVED:
			case MENU_EVENT_STATE_CHANGE:
				goto done;
			default:
				break;
		}

		menuActState();
	}

done:
	
	writeConfigFile();

	menuActState();
	menuSetReturnToState(STATE_PREVIOUS);
	
	return true;
}

void calcConsoleMetrics(void)
{
	int longestLen = 0;
	int curLen;
	const int maxLen = con_bottom.windowWidth;
	
	for(unsigned int i=0; i<arrayEntries(optionStrings); i++)
	{
		if((curLen = strlen(optionStrings[i])) > longestLen)
			longestLen = curLen;
	}
	
	consoleX = (maxLen - longestLen) / 2;
	consoleY = (con_bottom.windowHeight - arrayEntries(optionStrings) - MAX_SUBOPTIONS) / 2;
}

void rewindConsole(void)
{
	consoleSetCursor(&con_bottom, consoleX, consoleY);
	consoleSelect(&con_bottom);
}

void rewindConsoleX(void)
{
	unsigned int len = con_bottom.windowWidth - con_bottom.cursorX;
	while(len--) printf(" ");
	
	consoleSetCursor(&con_bottom, consoleX, con_bottom.cursorY + 1);
}

void handleBootMode(void)
{
	if(subOptionsActive)
	{
		u32 mode = (u32) curSubOptionIndex;
		configSetKeyData(KBootMode, &mode);
		curHighlightedSubOptionIndex = curSubOptionIndex;
		curSubOptionHighlighted = true;
	}
	else
	{
		u32 *mode = (u32 *)configGetData(KBootMode);
		if(mode)
		{
			curHighlightedSubOptionIndex = *mode;
			curSubOptionHighlighted = true;
		}
		else curSubOptionHighlighted = false;
		
		strcpy(optionSubStrings[0], "Normal");
		strcpy(optionSubStrings[1], "Quick Boot");
		strcpy(optionSubStrings[2], "Quiet Boot");
		curSubOptionCount = 3;
	}
}

void handleBootOption(void)
{
	if(subOptionsActive)
	{
		int key = KBootOption1 + (int) curSubOptionIndex;
		
		// const char *curPath = (const char *) configGetData(key);
		
		menuSetEnterNextState(MENU_STATE_BROWSER);
		menuUpdateGlobalState();
		menuActState();
		const char *path = browseForFile("sdmc:");
		uiClearConsoles();
		
		if(path)
			configSetKeyData(key, path);
	}
	else
	{
		strcpy(optionSubStrings[0], "[Slot 1]");
		strcpy(optionSubStrings[1], "[Slot 2]");
		strcpy(optionSubStrings[2], "[Slot 3]");
		curSubOptionCount = 3;
	}
}

void handleNandImage(void)
{
	if(subOptionsActive)
	{
		int ret;
		int key = KBootOption1NandImage + (int) curSubOptionIndex;
		
		// const char *curPath = (const char *) configGetData(key);
		
		menuSetEnterNextState(MENU_STATE_BROWSER);
		menuUpdateGlobalState();
		menuActState();
		const char *path = browseForFile("sdmc:");
		uiClearConsoles();
		
		if(path)
		{
			if(ret = validateNandImage(path))
			{
				switch(ret)
				{
					case NANDIMG_ERROR_BADPATH:
						menuPrintPrompt("Unsupported device or filepath!\n");
						break;
					case NANDIMG_ERROR_NEXISTS:
						menuPrintPrompt("NAND Image file corrupted!\n");
						break;
					case NANDIMG_ERROR_NCONTS:
						menuPrintPrompt("NAND Image file is not continuous!\n");
						break;
				}
			}
			else configSetKeyData(key, path);
		}
	}
	else
	{
		strcpy(optionSubStrings[0], "[Slot 1] Image");
		strcpy(optionSubStrings[1], "[Slot 2] Image");
		strcpy(optionSubStrings[2], "[Slot 3] Image");
		curSubOptionCount = 3;
	}
}
