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
#include "firm.h"
#include "hid.h"
#include "main.h"
#include "spiflash.h"

// PrintConsole for each screen
PrintConsole con_top, con_bottom;
// SD card FAT fs instance
FATFS sd_fs;
// same for all NAND fss
FATFS nand_twlnfs, nand_twlpfs, nand_fs;
// 
enum menu_state_type menu_state;

enum menu_state_type menu_previous_states[8];
int previous_states_count;

bool unit_is_new3ds;

void heap_init();

static void menu_main_draw_top()
{
	const char *sd_res[2] = {"\x1B[31mNo ", "\x1B[32mYes"};
	const char *nand_res[2] = {"\x1B[31mError ", "\x1B[32mOK   "};
	bool sd_status, nand_status;
	
	sd_status = dev_sdcard->is_active();
	nand_status = dev_rawnand->is_active();
	
	consoleSelect(&con_top);
	printf("\n\t\t\t\t\t3DS Bootloader v0.1\n\n\n\n");
	printf(" SD card inserted: %s\e[0m\n", sd_res[sd_status]);
	printf(" NAND status: %s\e[0m\n", nand_res[nand_status]);
	printf(" Wifi flash status: %s\e[0m", nand_res[spiflash_get_status()]);
}

int main(void)
{
	u32 keys;

	//Initialize console for both screen using the two different PrintConsole we have defined
	consoleInit(1, &con_top);
	consoleInit(0, &con_bottom);
	
	consoleSelect(&con_top);
	
	printf("\x1B[32mGood morning, hello!\e[0m\n\n");
	
	unit_detect();
	
	printf("Initializing devices...\n");
	
	devs_init();
	
	mount_fs();
	
	consoleClear();
	
	previous_states_count = 0;
	
	int cursor_pos = 0;
	
	// Menu main loop
	for(;;)
	{
		hidScanInput();
		keys = hidKeysDown();
		
		// get ready to repaint
		consoleSelect(&con_top);
		printf("\033[0;0H");	// set cursor to the upper left corner
		consoleSelect(&con_bottom);
		printf("\033[0;0H\n\n\n\n");
		
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
				else firm_launch(NULL);
				break;
			
			default: printf("OOPS!\n"); break;
		}
		
		
	}

	return 0;
}

void heap_init()
{
	extern void* fake_heap_start;
	extern void* fake_heap_end;
	fake_heap_start = (void*)A9_HEAP_START;
	fake_heap_end = (void*)A9_HEAP_END;
}

void devs_init()
{
	bool res = true;
	bool flash_res;
	const char *res_str[2] = {"\x1B[31mFailed!", "\x1B[32mOK!"};
	
	printf(" Initializing SD card... ");
	printf("%s\e[0m\n", res_str[dev_sdcard->init()]);
	
	printf(" Initializing NAND... ");
	res &= dev_rawnand->init();
	printf("%s\e[0m\n", res_str[res]);
	
	// only if nand init was successful
	if(res)
	{
		printf(" Identifying NAND partitions... \n");
		printf(" %s\e[0m\n", res_str[dev_decnand->init()]);
	}
	
	printf(" Initializing Wifi flash... ");
	flash_res = dev_flash->init();
	printf("%s\e[0m\n", res_str[flash_res]);
	res &= flash_res;
	
	if(!res) sleep_wait(0x8000000); // mmc or wififlash init fail
}

void mount_fs()
{
	FRESULT res;
	const char *res_str[2] = {"\x1B[31mFailed!", "\x1B[32mOK!"};

	printf("Mounting SD card FAT FS... ");
	res = f_mount(&sd_fs, "sdmc:", 1);
	if(res == FR_OK) printf("%s\e[0m\n", res_str[1]);
	else printf("%s ERROR 0x%X\e[0m\n", res_str[0], res);

	printf("Mounting twln FS... ");
	res = f_mount(&nand_twlnfs, "twln:", 1);
	if(res == FR_OK) printf("%s\e[0m\n", res_str[1]);
	else printf("%s ERROR 0x%X\e[0m\n", res_str[0], res);

	printf("Mounting twlp FS... ");
	res = f_mount(&nand_twlpfs, "twlp:", 1);
	if(res == FR_OK) printf("%s\e[0m\n", res_str[1]);
	else printf("%s ERROR 0x%X\e[0m\n", res_str[0], res);

	printf("Mounting CTR NAND FAT FS... ");
	res = f_mount(&nand_fs, "nand:", 1);
	if(res == FR_OK) printf("%s\e[0m\n", res_str[1]);
	else printf("%s ERROR 0x%X\e[0m\n", res_str[0], res);
}

void unit_detect()
{
	printf("Detecting unit... ");
	
	if(PDN_MPCORE_CLKCNT != 0)
		unit_is_new3ds = true;
		
	printf("%s3DS detected!\n", unit_is_new3ds ? "New " : "");
}

static void loadFirmNand(void)
{
	memset((u8*)FCRAM_BASE, 0x00, 0x200);
	dev_decnand->read(0x0005A980, 0x00002000, (u8*)FCRAM_BASE);
}

static bool loadFirmSd(const char *filePath)
{
	FIL file;
	UINT bytesRead = 0;
	bool res = true;


	if(f_open(&file, filePath, FA_READ) != FR_OK)
	{
		printf("Failed to open '%s'!\n", filePath);
		return false;
	}
	if(f_read(&file, (u8*)FIRM_LOAD_ADDR, FIRM_MAX_SIZE, &bytesRead) != FR_OK)
	{
		printf("Failed to read from file!\n");
		res = false;
	}
	f_close(&file);

	return res;
}

