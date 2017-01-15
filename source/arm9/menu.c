#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "mem_map.h"
#include "io.h"
#include "util.h"
#include "pxi.h"
#include "arm9/console.h"
#include "arm9/dev.h"
#include "fatfs/ff.h"
#include "fatfs/diskio.h"
#include "hid.h"
#include "version.h"
#include "arm9/main.h"
#include "arm9/util_nand.h"
#include "arm9/menu.h"
#include "arm9/timer.h"
#include "arm9/interrupt.h"
#include "arm9/config.h"

// PrintConsole for each screen
extern PrintConsole con_top, con_bottom;

const menu_state_options menu_main = {
	5,
	{
		{"Launch FIRM", MENU_STATE_FIRM_LAUNCH_SETTINGS},
		{"NAND tools...", MENU_STATE_NAND_MENU},
		{"Options...", MENU_STATE_OPTIONS_MENU},
		{"Browse FIRM...", MENU_STATE_BROWSER},
		{"CONFIG TEST", MENU_STATE_TEST_CONFIG}
	}
};

const menu_state_options menu_nand = {
	2,
	{
		{"Backup NAND", MENU_STATE_NAND_BACKUP},
		{"Restore NAND", MENU_STATE_NAND_RESTORE}
	}
};

// menu_state_type -> menu_state_options instance
const menu_state_options *options_lookup[] = {
	&menu_main, // STATE_MAIN
	&menu_nand, // STATE_NAND_MENU
};

enum menu_state_type menu_state;
enum menu_state_type menu_next_state;

enum menu_state_type menu_previous_states[8];
int menu_previous_states_count;

int menu_event_state;


u8 color;

static void menu_main_draw_top()
{
	const char *sd_res[2]	= {"\x1B[31mNo ", "\x1B[32mYes"};
	const char *nand_res[2]	= {"\x1B[31mError ", "\x1B[32mOK   "};
	
	consoleSelect(&con_top);
	drawConsoleWindow(&con_top, 2, color);
	printf("\n\t\t\t\t\t3DS Bootloader v%" PRIu16 ".%" PRIu16 "\n\n\n\n",
			BOOTLOADER_VERSION>>16, BOOTLOADER_VERSION & 0xFFFFu);
	
	printf(" Model: %s\n", bootInfo.model);
	printf(" \x1B[33m%s\e[0m\n", bootInfo.boot_env);
	printf(" \x1B[32m(%s Mode)\e[0m\n\n", bootInfo.mode);
	printf(" SD card inserted: %s\e[0m\n", sd_res[bootInfo.sd_status]);
	printf(" NAND status: %s\e[0m\n", nand_res[bootInfo.nand_status]);
	printf(" Wifi flash status: %s\e[0m", nand_res[bootInfo.wififlash_status]);
}

static void rewindConsole()
{
	// get ready to repaint
	consoleSelect(&con_top);
	printf("\033[0;0H");	// set cursor to the upper left corner
	consoleSelect(&con_bottom);
	printf("\033[0;0H\n\n\n\n");
}

int enter_menu(int initial_state)
{
	u32 keys;
	int cursor_pos;
	const char* path;
	const char *firmPath = NULL;
	
	menu_event_state = MENU_EVENT_NONE;

	// randomize color
	color = (rng_get_byte() % 6) + 1;
	
	cursor_pos = 0;
	
	clearConsoles();

	TIMER_start(TIMER_0, TIMER_PRESCALER_64, TIMER_FREQ_64(60.0f), NULL);

	// caller requested to enter a submenu?
	if(initial_state != MENU_STATE_MAIN)
	{
		menuRunOnce(initial_state);
		goto exitAndLaunchFirm;
	}
	
	menuSetEnterNextState(MENU_STATE_MAIN);
	menuActState();
	

	// Menu main loop
	for(;;)
	{
		const menu_state_options *cur_options = options_lookup[menu_state];
		
		switch(menu_state)
		{
			case MENU_STATE_MAIN:
			case MENU_STATE_NAND_MENU:
				
				hidScanInput();
				keys = hidKeysDown();
		
				rewindConsole();
				
				menu_main_draw_top();
			
				// print all the options of the current state
				consoleSelect(&con_bottom);
				for(int i=0; i < cur_options->count; i++)
					printf("\t\t\t%s\e[0m %s\n", cursor_pos == i ? "\x1B[33m*" : " ",
					cur_options->options[i].name);

				if(keys & KEY_DUP)
				{
					if(cursor_pos == 0) cursor_pos = cur_options->count - 1;	// jump to the last option
					else cursor_pos--;
				}
				else if(keys & KEY_DDOWN)
				{
					if(cursor_pos == cur_options->count - 1) cursor_pos = 0;	// jump to the first option
					else cursor_pos++;
				}
				else if(keys & KEY_A)	// select option
				{
					menuSetEnterNextState(cur_options->options[cursor_pos].state);
					cursor_pos = 0;
				}
				else if(keys & KEY_B)	// go back
				{
					menuSetReturnToState(STATE_PREVIOUS);
					cursor_pos = 0;
				}
				break;
				
			case MENU_STATE_NAND_BACKUP:
				menuDumpNand("sdmc:/nand.bin");
				clearConsoles();
				break;
				
			case MENU_STATE_NAND_RESTORE:
				menuRestoreNand("sdmc:/nand.bin");
				clearConsoles();
				break;
				
			case MENU_STATE_OPTIONS_MENU:
				menuOptions();
				clearConsoles();
				break;
			
			case MENU_STATE_FIRM_LAUNCH:
				if(!menuLaunchFirm(firmPath, false))
					clearConsoles();
				break;
				
			case MENU_STATE_FIRM_LAUNCH_SETTINGS:
				if(!menuTryLoadFirmwareFromSettings(false))
					clearConsoles();
				break;
				
			case MENU_STATE_BROWSER:
				path = browseForFile("sdmc:");
				clearConsoles();
				firmPath = path;
				if(!path)	// no file selected
					break;
				menuSetEnterNextState(MENU_STATE_FIRM_LAUNCH);
				break;
			
			case MENU_STATE_TEST_CONFIG:
				clearConsoles();
				consoleSelect(&con_top);
				printf("Key Text Data:\n");
				for(int i=0; i<KLast; i++)
				{
					char *text = (char*) configCopyText(i);
					printf("\n%s: ", configGetKeyText(i));
					if(text) {
						printf("%s", text);
						free(text);
					}
					else printf("<invalid>");
				}
				printf("\n\nKey Raw Data:\n");
				for(int i=0; i<KLast; i++)
				{
					u8 *data = (u8*) configGetData(i);
					printf("\n%s: ", configGetKeyText(i));
					if(data) {
						if(i < KBootOption1Buttons)
							printf("filepath: %s", (char*)data);
						else if(i < KBootMode)
							printf("pad val: 0x%lX", *(u32*)data);
						else
							printf("boot mode val: 0x%lX", *(u32*)data);
					}
					else printf("<invalid>");
				}
				TIMER_sleep(6000);
				clearConsoles();
				menuSetReturnToState(STATE_PREVIOUS);
				break;
			
			case MENU_STATE_EXIT:
				goto exitAndLaunchFirm;
				break;
			
			default: printf("OOPS!\n"); break;
		}

		switch(menuUpdateGlobalState())
		{
			case MENU_EVENT_HOME_PRESSED:
				break;
			case MENU_EVENT_STATE_CHANGE:
				if(menu_next_state == MENU_STATE_EXIT)
					goto exitAndLaunchFirm;
				else
					clearConsole(0);
				break;
			default:
				break;
		}

		menuActState();
	}

exitAndLaunchFirm:
	TIMER_stop(TIMER_0);

	return 0;
}

void menuRunOnce(int state)
{
	switch(state)
	{
		case MENU_STATE_FIRM_LAUNCH_SETTINGS:
			if(!menuTryLoadFirmwareFromSettings())
				clearConsoles();
			break;
		
		default:
			panic();
	}
}

void menuSetReturnToState(int state)
{
	int i;

	if(state == STATE_PREVIOUS)
	{
		if(menu_previous_states_count == 0)
			return;
		state = menu_previous_states[menu_previous_states_count - 1];
		menu_previous_states_count--;
	}
	else if(state == MENU_STATE_EXIT)
	{
		menu_previous_states_count = 0;
	}
	else
	{
		for(i=menu_previous_states_count; i>0; i--)
		{
			if(menu_previous_states[i] == state)
			{
				menu_previous_states_count = i;
			}
		}
	}

	menu_next_state = state;
	// emit event
	menu_event_state = MENU_EVENT_STATE_CHANGE;
}

void menuSetEnterNextState(int state)
{
	if(menu_previous_states_count >= 8)
		panic();

	menu_previous_states[menu_previous_states_count] = menu_state;
	menu_previous_states_count++;

	menu_next_state = state;
	// emit event
	menu_event_state = MENU_EVENT_STATE_CHANGE;
}

int menuUpdateGlobalState(void)
{
	int retcode = MENU_EVENT_NONE;

	// Later if PXI interrupts are implemented we need an
	// interrupt handler here which can tell the timer and
	// PXI interrupts apart.
	//waitForIrq();


	/* Check PXI Response register */

	bool successFlag;
	u32 replyCode = PXI_tryRecvWord(&successFlag);

	while(successFlag)
	{
		switch(replyCode)
		{
			case PXI_RPL_HOME_PRESSED:
				retcode = MENU_EVENT_HOME_PRESSED;
				break;
			case PXI_RPL_POWER_PRESSED:
				retcode = MENU_EVENT_POWER_PRESSED;
				break;
			default:
				panic();
		}
		// maybe there's more..?
		replyCode = PXI_tryRecvWord(&successFlag);
	}


	/* Check for HW changes */

	bool sd_status;

	sd_status = dev_sdcard->is_active();
	if(sd_status != bootInfo.sd_status)
	{
		retcode = sd_status ? MENU_EVENT_SD_CARD_INSERTED :
				MENU_EVENT_SD_CARD_REMOVED;
		bootInfo.sd_status = sd_status;	// update bootInfo status
	}


	// Submenu wants to call a new menu?
	if(menu_state != menu_next_state)
		retcode = MENU_EVENT_STATE_CHANGE;

	// did anything happen?
	if(retcode != MENU_EVENT_NONE)
		menu_event_state = retcode;

	return retcode;
}

void menuActState(void)
{
	switch(menu_event_state)
	{
		case MENU_EVENT_NONE:
			break;	// nothing to do, boring.
		case MENU_EVENT_POWER_PRESSED:
			power_off_safe();
			break;
		case MENU_EVENT_HOME_PRESSED:
			menuSetReturnToState(MENU_STATE_MAIN);
			break;
		case MENU_EVENT_STATE_CHANGE:
			menu_state = menu_next_state;
			break;
		case MENU_EVENT_SD_CARD_INSERTED:
			// try to initialize sd card
			dev_sdcard->init();
			f_mount(&sd_fs, "sdmc:", 1);
			// also try to load saved settings
			loadConfigFile();
			break;
		case MENU_EVENT_SD_CARD_REMOVED:
			// what else to do here?
			dev_sdcard->close();
			f_mount(NULL, "sdmc:", 1);
			break;
		default:
			break;
	}

	menu_event_state = MENU_EVENT_NONE;
}
/*
static char *menuNormalizeText(const char *text)
{
	size_t curLineLen;
	const char *p = text;
	
	char *q = (char *) malloc((strlen(text) + 1) * 3);
	if(!q)
		return NULL;
	
	while(*p)
	{
		if(p[0] == '\\' && p[1] == 'n')
		{
			
		}
	}
}


void menuCalcWindowMetrics(const char *text, int *x, int *y, int *width, int *height)
{
	const char *p = text;
	size_t longestLineLen = 0;
	size_t curLineLen;
	size_t lines = 0;
	
	while(*p)
	{
		// Note: text shouldn't be malformed.
		if(p[0] == '\n')
		{
			lines ++;
			if(curLineLen > longestLineLen)
				longestLineLen = curLineLen;
			p += 2;
		}
		else
		{
			curLineLen++;
			p++;
		}
	}
	
	if(curLineLen > longestLineLen)
		longestLineLen = curLineLen;
	
	
}
*/
void menuPrintPrompt(const char *text)
{
	/*
	const char *p = text;
	size_t lines = 0;
	
	
	while(*p)
	{
	
	}*/
	
	printf(text);
}

void menuWaitForAnyPadkey()
{
	int keys;
	bool finished = false;
	
	do {
		hidScanInput();
		keys = hidKeysDown();
		if(keys & HID_KEY_MASK_ALL)
			finished = true;
		
		switch(menuUpdateGlobalState())
		{
			case MENU_EVENT_HOME_PRESSED:
			case MENU_EVENT_POWER_PRESSED:
				finished = true;
			default:
				break;
		}
		
		menuActState();
		
	} while(!finished);
}
