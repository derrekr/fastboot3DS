#include <stdio.h>
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
// same for NAND
FATFS nand_fs;
// 
enum menu_state_type menu_state;

bool unit_is_new3ds;

void menu_main_draw_top()
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
		
		switch(menu_state)
		{
		
			case STATE_MAIN:
				if((cursor_pos != 0) && (keys & KEY_DUP)) cursor_pos--;
				else if((cursor_pos != 1) && (keys & KEY_DDOWN)) cursor_pos++;
				else if(keys & KEY_A)
				{
					if(cursor_pos == 0) menu_state = STATE_FIRM_LAUNCH;
				}
				consoleSelect(&con_bottom);
				for(int i=0; i<2; i++)
					printf("\t\t\t%s\e[0m %s\n", cursor_pos == i ? "\x1B[33m*" : " ", menu_main_slots[i]);
				menu_main_draw_top();
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
			
			default: ;
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
	
	sd_fs.drv = FATFS_DEV_NUM_SD;
	nand_fs.drv = FATFS_DEV_NUM_NAND;
	
	printf("Mounting SD card FAT FS... ");
	res = f_mount(&sd_fs, "sd", 1);
	if(res == FR_OK) printf("%s\e[0m\n", res_str[1]);
	else printf("%s ERROR 0x%X\e[0m\n", res_str[0], res);
	
	printf("Mounting CTR NAND FAT FS... ");
	f_mount(&nand_fs, "nand", 1);
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

void loadFirmNand(void)
{
	memset((u8*)FCRAM_BASE, 0x00, 0x200);
	dev_decnand->read(0x0005A980, 0x00002000, (u8*)FCRAM_BASE);
}

bool loadFirmSd(const char *filePath)
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

