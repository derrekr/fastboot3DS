#include <stdio.h>
#include <string.h>
#include "types.h"
#include "mem_map.h"
#include "IO.h"
#include "util.h"
#include "console.h"
#include "dev.h"
#include "fatfs/ff.h"
#include "fatfs/diskio.h"
#include "hid.h"
#include "main.h"
#include "util_nand.h"
#include "menu.h"

u8 color;
enum menu_state_type menu_state;

enum menu_state_type menu_previous_states[8];
int previous_states_count;

bool unit_is_new3ds;

void heap_init();

static void menu_main_draw_top()
{
	const char *sd_res[2]	= {"\x1B[31mNo ", "\x1B[32mYes"};
	const char *nand_res[2]	= {"\x1B[31mError ", "\x1B[32mOK   "};
	const char *unit[2]		= {"Original 3DS", "New 3DS"};
	const char *boot_environment [2]	=	{	"Cold boot",				// 0
												"Booted from Native FIRM",	// 1
												"Booted from <Unknown>",	// 2, etc
												"Booted from Legacy FIRM"	// 3
											};
	
	bool sd_status, nand_status, wififlash_status;
	
	sd_status = dev_sdcard->is_active();
	nand_status = dev_rawnand->is_active();
	wififlash_status = dev_flash->is_active();
	
	consoleSelect(&con_top);
	drawConsoleWindow(&con_top, 2, color);
	printf("\n\t\t\t\t\t3DS Bootloader v0.1\n\n\n\n");
	
	printf(" Model: %s\n", unit[unit_is_new3ds]);
	printf(" \x1B[33m%s\e[0m\n\n", boot_environment[boot_env]);
	printf(" SD card inserted: %s\e[0m\n", sd_res[sd_status]);
	printf(" NAND status: %s\e[0m\n", nand_res[nand_status]);
	printf(" Wifi flash status: %s\e[0m", nand_res[wififlash_status]);
}

void rewindConsole()
{
	// get ready to repaint
	consoleSelect(&con_top);
	printf("\033[0;0H");	// set cursor to the upper left corner
	consoleSelect(&con_bottom);
	printf("\033[0;0H\n\n\n\n");
}

void clearConsoles()
{
	consoleSelect(&con_bottom);
	consoleClear();
	consoleSelect(&con_top);
	consoleClear();
}

int enter_menu(void)
{
	u32 keys;
	int cursor_pos;
	
	// randomize color
	color = (rng_get_byte() % 6) + 1;
	
	previous_states_count = 0;
	
	cursor_pos = 0;
	
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
				if(!firm_load_verify())
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
			
			default: printf("OOPS!\n"); break;
		}
		
		
	}

exitAndLaunchFirm:

	return 0;
}
