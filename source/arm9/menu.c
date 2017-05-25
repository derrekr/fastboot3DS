#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "mem_map.h"
#include "util.h"
#include "pxi.h"
#include "arm9/console.h"
#include "arm9/dev.h"
#include "fatfs/ff.h"
#include "fatfs/diskio.h"
#include "hid.h"
#include "version.h"
#include "arm9/main.h"
#include "arm9/menu.h"
#include "arm9/menu_firmloader.h"
#include "arm9/timer.h"
#include "arm9/interrupt.h"
#include "arm9/config.h"


const menu_state_options menu_main = {
	6,
	{
		{"Continue boot", MENU_STATE_FIRM_LAUNCH_SETTINGS},
		{"Launch Firmware...", MENU_STATE_BROWSER},
		{"NAND tools...", MENU_STATE_NAND_MENU},
		{"Options...", MENU_STATE_OPTIONS_MENU},
		{"Update", MENU_STATE_UPDATE},
		{"Credits", MENU_STATE_CREDITS},
	}
};

const menu_state_options menu_nand = {
	3,
	{
		{"Backup NAND", MENU_STATE_NAND_BACKUP},
		{"Restore NAND", MENU_STATE_NAND_RESTORE},
		{"Flash Firmware", MENU_STATE_NAND_FLASH_FIRM}
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
static bool waitForVBlank;

static void menuRunOnce(int state);

static void menu_main_draw_top()
{
	const char *sd_res[2]	= {"\x1B[31mNo ", "\x1B[32mYes"};
	const char *nand_res[2]	= {"\x1B[31mError ", "\x1B[32mOK   "};
	
	consoleSelect(&con_top);
	
	uiPrintCenteredInLine(1, "fastboot 3DS v%" PRIu16 ".%" PRIu16,
			BOOTLOADER_VERSION>>16, BOOTLOADER_VERSION & 0xFFFFu);
	
	uiPrintTextAt(1, 4, "Model: %s", bootInfo.model);
	uiPrintTextAt(1, 5, "\x1B[33m%s\e[0m", bootInfo.boot_env);
	uiPrintTextAt(1, 6, "\x1B[32m(%s Mode)\e[0m", bootInfo.mode);
	uiPrintTextAt(1, 8, "SD card inserted: %s\e[0m", sd_res[bootInfo.sd_status]);
	uiPrintTextAt(1, 9, "NAND status: %s\e[0m", nand_res[bootInfo.nand_status]);
	uiPrintTextAt(1, 10, "Wifi flash status: %s\e[0m", nand_res[bootInfo.wififlash_status]);
}

void menuSetVBlank(bool enable)
{
	if(enable)
	{
		IRQ_registerHandler(IRQ_TIMER_0, NULL);
		TIMER_start(TIMER_0, TIMER_PRESCALER_64, TIMER_FREQ_64(60.0f), true);
		waitForVBlank = true;
	}
	else
	{
		waitForVBlank = false;
		TIMER_stop(TIMER_0);
		IRQ_unregisterHandler(IRQ_TIMER_0);
	}
}

int enter_menu(int initial_state)
{
	u32 keys;
	int cursor_pos;
	const char* path;
	const char *firmPath = NULL;
	
	menu_event_state = MENU_EVENT_NONE;
	
	cursor_pos = 0;
	
	uiClearConsoles();
	uiDrawConsoleWindow();

	menuSetVBlank(true);

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
				
				menu_main_draw_top();
			
				// print all the options of the current state
				consoleSelect(&con_bottom);
				for(int i=0; i < cur_options->count; i++)
					uiPrintTextAt(9, 4+i, "%s\e[0m %s\n", cursor_pos == i ? "\x1B[33m*" : " ",
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
				menuSetVBlank(false);
				menuDumpNand("sdmc:/nand.bin");
				uiClearConsoles();
				menuSetVBlank(true);
				break;
				
			case MENU_STATE_NAND_RESTORE:
				menuSetVBlank(false);
				menuRestoreNand("sdmc:/nand.bin");
				uiClearConsoles();
				menuSetVBlank(true);
				break;
			
			case MENU_STATE_NAND_FLASH_FIRM:
				path = browseForFile("sdmc:");
				uiClearConsoles();
				if(!path)
					break;
				menuSetVBlank(false);
				menuFlashFirmware(path);
				uiClearConsoles();
				menuSetVBlank(true);
				break;
			
			case MENU_STATE_OPTIONS_MENU:
				menuOptions();
				uiClearConsoles();
				break;
			
			case MENU_STATE_FIRM_LAUNCH:
				if(!menuLaunchFirm(firmPath, false))
					uiClearConsoles();
				break;
				
			case MENU_STATE_FIRM_LAUNCH_SETTINGS:
				if(!tryLoadFirmwareFromSettings(true))
					uiClearConsoles();
				break;
				
			case MENU_STATE_BROWSER:
				path = browseForFile("sdmc:");
				uiClearConsoles();
				firmPath = path;
				if(!path)	// no file selected
					break;
				menuSetEnterNextState(MENU_STATE_FIRM_LAUNCH);
				break;
			
			case MENU_STATE_UPDATE:
				menuSetVBlank(false);
				menuUpdateLoader();
				uiClearConsoles();
				menuSetVBlank(true);
				break;
			
			case MENU_STATE_CREDITS:
				menuCredits();
				uiClearConsoles();
				break;
			
			case MENU_STATE_EXIT:
				goto exitAndLaunchFirm;
				break;
			
			/* this should never happen */
			default: uiPrintIfVerbose("OOPS!\n"); break;
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
	menuSetVBlank(false);

	return 0;
}

static void menuRunOnce(int state)
{
	switch(state)
	{
		case MENU_STATE_FIRM_LAUNCH_SETTINGS:
			if(!tryLoadFirmwareFromSettings(true))
				uiClearConsoles();
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
	if(waitForVBlank) waitForIrq();


	/* Check PXI Response register */

	bool successFlag;
	u32 replyCode = PXI_tryRecvWord(&successFlag);

	while(successFlag)
	{
		switch(replyCode)
		{
			case PXI_RPL_HOME_PRESSED:
			case PXI_RPL_HOME_HELD:
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

void menuPrintPrompt(const char *text)
{
	// TODO: use uiShowMessageWindow
	uiPrintInfo(text);
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
