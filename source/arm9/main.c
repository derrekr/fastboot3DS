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
u8 color;
// SD card FAT fs instance
FATFS sd_fs;
// same for all NAND fss
FATFS nand_twlnfs, nand_twlpfs, nand_fs;

bool	unit_is_new3ds;
u8		boot_env;

int main(void)
{
	u32 keys;
	
	hashCodeRoData();	// TODO: remove after debugging

	//Initialize console for both screen using the two different PrintConsole we have defined
	consoleInit(SCREEN_TOP, &con_top);
	consoleSetWindow(&con_top, 1, 1, con_top.windowWidth - 2, con_top.windowHeight - 2);
	consoleInit(SCREEN_LOW, &con_bottom);
	
	consoleSelect(&con_top);
	
	printf("\x1B[32mGood morning, hello!\e[0m\n\n");
	
	unit_detect();
	
	printf("Initializing devices...\n");
	
	devs_init();
	
	printf("Mounting filesystems...\n");
	
	mount_fs();
	
	consoleClear();
	
	enter_menu();

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

void devs_close()
{
	dev_sdcard->close();
	dev_decnand->close();
	dev_rawnand->close();
	dev_flash->close();
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
	sleep_wait(0x8000000);
}

void unmount_fs()
{
	f_mount(NULL, "sdmc:", 1);
	f_mount(NULL, "twln:", 1);
	f_mount(NULL, "twlp:", 1);
	f_mount(NULL, "nand:", 1);
}

void unit_detect()
{
	printf("Detecting unit... ");
	
	if(PDN_MPCORE_CLKCNT != 0)
		unit_is_new3ds = true;
		
	bool is_panda = CFG_UNITINFO != 0;
		
	printf("%s%s 3DS detected!\n", is_panda ? "Dev " : "",
									unit_is_new3ds ? "New" : "Original");
}

void boot_env_detect()
{
	boot_env = CFG_BOOTENV;
	if(boot_env > 3) boot_env = 2;
}

u8 rng_get_byte()
{	
	return *((u32*)0x10011000) & 0xFF; // PRNG reg
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

