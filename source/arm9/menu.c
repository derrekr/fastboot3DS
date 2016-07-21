#include <stdio.h>
#include <string.h>
#include "types.h"
#include "mem_map.h"
#include "io.h"
#include "util.h"
#include "arm9/console.h"
#include "arm9/dev.h"
#include "arm9/fatfs/ff.h"
#include "arm9/fatfs/diskio.h"
#include "hid.h"
#include "arm9/main.h"
#include "arm9/util_nand.h"
#include "arm9/menu.h"
#include "arm9/timer.h"
#include "arm9/interrupt.h"

u8 color;
enum menu_state_type menu_state;

enum menu_state_type menu_previous_states[8];
int previous_states_count;

void heap_init();

static void menu_main_draw_top()
{
	const char *sd_res[2]	= {"\x1B[31mNo ", "\x1B[32mYes"};
	const char *nand_res[2]	= {"\x1B[31mError ", "\x1B[32mOK   "};
	
	bool sd_status;
	
	sd_status = dev_sdcard->is_active();
	
	consoleSelect(&con_top);
	drawConsoleWindow(&con_top, 2, color);
	printf("\n\t\t\t\t\t3DS Bootloader v0.1\n\n\n\n");
	
	printf(" Model: %s\n", bootInfo.model);
	printf(" \x1B[33m%s\e[0m\n", bootInfo.boot_env);
	printf(" \x1B[32m(%s Mode)\e[0m\n\n", bootInfo.mode);
	printf(" SD card inserted: %s\e[0m\n", sd_res[sd_status]);
	printf(" NAND status: %s\e[0m\n", nand_res[bootInfo.nand_status]);
	printf(" Wifi flash status: %s\e[0m", nand_res[bootInfo.wififlash_status]);
}

void rewindConsole()
{
	// get ready to repaint
	consoleSelect(&con_top);
	printf("\033[0;0H");	// set cursor to the upper left corner
	consoleSelect(&con_bottom);
	printf("\033[0;0H\n\n\n\n");
}

int enter_menu(void)
{
	u32 keys;
	int cursor_pos;
	
	// randomize color
	color = (rng_get_byte() % 6) + 1;
	
	previous_states_count = 0;
	
	cursor_pos = 0;

	startTimer(TIMER_0, TIMER_PRESCALER_64, TIMER_FREQ_64(60.0f), true);

	// Menu main loop
	for(;;)
	{
		hidScanInput();
		keys = hidKeysDown();
		
		rewindConsole();
		
		const menu_state_options *cur_options = options_lookup[menu_state];
		
		switch(menu_state)
		{
		
			case STATE_MAIN:
			case STATE_NAND_MENU:
			
				menu_main_draw_top();
			
				// print all the options of the current state
				consoleSelect(&con_bottom);
				for(int i=0; i < cur_options->count; i++)
					printf("\t\t\t%s\e[0m %s\n", cursor_pos == i ? "\x1B[33m*" : " ", cur_options->options[i].name);

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
					menu_previous_states[previous_states_count] = menu_state;
					previous_states_count++;
					menu_state = cur_options->options[cursor_pos].state;
					consoleClear();
					cursor_pos = 0;
				}
				else if(keys & KEY_B)	// go back
				{
					if(previous_states_count != 0)
					{
						menu_state = menu_previous_states[previous_states_count-1];
						previous_states_count--;
						consoleClear();
						cursor_pos = 0;
					}
				}
				break;
				
			case STATE_NAND_BACKUP:
				consoleSelect(&con_top);
				consoleClear();
				dumpNand("sdmc:/nand.bin");
				clearConsoles();
				menu_state = menu_previous_states[previous_states_count-1];
				previous_states_count--;
				break;
				
			case STATE_NAND_RESTORE:
				consoleSelect(&con_top);
				consoleClear();
				restoreNand("sdmc:/nand.bin");
				clearConsoles();
				menu_state = menu_previous_states[previous_states_count-1];
				previous_states_count--;
				break;
			
			case STATE_FIRM_LAUNCH:
				consoleClear();
				consoleSelect(&con_top);
				consoleClear();
				if(!firm_load_verify(0x400000))
				{
					consoleSelect(&con_bottom);
					printf("Press B to return to Main Menu");
					do {
						hidScanInput();
						keys = hidKeysDown();
					} while(!(keys & KEY_B));
					menu_state = STATE_MAIN;
				}
				else goto exitAndLaunchFirm;
				break;
				
				// test
			case STATE_TEST_MENU:
				consoleClear();
				consoleSelect(&con_top);
				consoleClear();
				const char* path = browseForFile("sdmc:");
				consoleClear();
				printf("selected file:\n%s\n", path);
				sleep_wait(0x8000000);
				printf("loading firm...\n");
				loadFirmSd(path);
				if(!firm_load_verify(0x400000)) printf("bad firm\n");
				else goto exitAndLaunchFirm;
				menu_state = STATE_MAIN;
				break;
			
			default: printf("OOPS!\n"); break;
		}

		// Later if PXI interrupts are implemented we need an
		// interrupt handler here which can tell the timer and
		// PXI interrupts apart.
		waitForIrq();
		REG_IRQ_IF = (u32)IRQ_TIMER_0;
	}

exitAndLaunchFirm:
	stopTimer(TIMER_0);

	return 0;
}

