#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "arm9/main.h"
#include "util.h"
#include "io.h"
#include "arm9/fatfs/ff.h"
#include "arm9/dev.h"
#include "arm9/firm.h"
#include "arm9/config.h"

static void devs_init();
static void mount_fs();
static void unit_detect();
static void boot_env_detect();
static void fw_detect();
static void loadSettings();

int main(void)
{
	hashCodeRoData();	// TODO: remove after debugging

	//Initialize console for both screen using the two different PrintConsole we have defined
	consoleInit(SCREEN_TOP, &con_top);
	consoleSetWindow(&con_top, 1, 1, con_top.windowWidth - 2, con_top.windowHeight - 2);
	consoleInit(SCREEN_LOW, &con_bottom);
	
	consoleSelect(&con_top);
	
	printf("\x1B[32mGood morning, hello!\e[0m\n\n");
	
	printf("Detecting unit...\n");
	
	unit_detect();
	
	printf("Detecting boot environment...\n");
	
	boot_env_detect();
	
	printf("Initializing devices...\n");
	
	devs_init();
	
	printf("Mounting filesystems...\n");
	
	mount_fs();
	
	printf("Detecting firmware...\n");
	
	fw_detect();
	
	printf("Loading settings...\n");
	
	loadSettings();
	
	wait(0x8000000);

	consoleClear();
	
	enter_menu();

	unmount_fs();
	
	devs_close();

	return 0;
}

static void devs_init()
{
	bool res;
	const char *res_str[2] = {"\x1B[31mFailed!", "\x1B[32mOK!"};

	printf(" Initializing SD card... ");
	printf("%s\e[0m\n", res_str[dev_sdcard->init()]);
	printf(" Initializing raw NAND... ");
	printf("%s\e[0m\n", res_str[dev_rawnand->init()]);
	printf(" Initializing Wifi flash... ");
	printf("%s\e[0m\n", res_str[(res = dev_flash->init())]);

	bootInfo.sd_status = dev_sdcard->is_active();
	bootInfo.nand_status = dev_rawnand->is_active();
	bootInfo.wififlash_status = dev_flash->is_active();

	if(!res) wait(0x8000000); // mmc or wififlash init fail
}

void devs_close()
{
	dev_sdcard->close();
	dev_decnand->close();
	dev_rawnand->close();
	dev_flash->close();
}

static void mount_fs()
{
	FRESULT res;
	const char *res_str[2] = {"\x1B[31mFailed!", "\x1B[32mOK!"};

	printf("Mounting SD card FAT FS... ");
	res = f_mount(&sd_fs, "sdmc:", 1);
	if(res == FR_OK) printf("%s\e[0m\n", res_str[1]);
	else printf("%s ERROR 0x%d\e[0m\n", res_str[0], res);

	printf("Mounting twln FS... ");
	res = f_mount(&nand_twlnfs, "twln:", 1);
	if(res == FR_OK) printf("%s\e[0m\n", res_str[1]);
	else printf("%s ERROR 0x%d\e[0m\n", res_str[0], res);

	printf("Mounting twlp FS... ");
	res = f_mount(&nand_twlpfs, "twlp:", 1);
	if(res == FR_OK) printf("%s\e[0m\n", res_str[1]);
	else printf("%s ERROR 0x%d\e[0m\n", res_str[0], res);

	printf("Mounting CTR NAND FAT FS... ");
	res = f_mount(&nand_fs, "nand:", 1);
	if(res == FR_OK) printf("%s\e[0m\n", res_str[1]);
	else printf("%s ERROR 0x%d\e[0m\n", res_str[0], res);
}

void unmount_fs()
{
	f_mount(NULL, "sdmc:", 1);
	f_mount(NULL, "twln:", 1);
	f_mount(NULL, "twlp:", 1);
	f_mount(NULL, "nand:", 1);
}

static void unit_detect()
{
	bootInfo.unit_is_new3ds = REG_PDN_MPCORE_CLKCNT != 0;

	sprintf(bootInfo.model, "%s 3DS", bootInfo.unit_is_new3ds ? "New" : "Original");
		
	printf("%s detected!\n", bootInfo.model);
}

static void boot_env_detect()
{
	static const char *boot_environment [4]	=	{	"Cold boot",				// 0
													"Booted from Native FIRM",	// 1
													"Booted from <Unknown>",	// 2, etc
													"Booted from Legacy FIRM"	// 3
												};

	u32 boot_env = CFG_BOOTENV;
	if(boot_env > 3) boot_env = 2;
	
	sprintf(bootInfo.mode, "%s", CFG_UNITINFO != 0 ? "Dev" : "Retail");
	
	strcpy(bootInfo.boot_env, boot_environment[boot_env]);
}

static void fw_detect()
{
	u8 *nand_sector = (u8*)malloc(0x200);

	if(!bootInfo.nand_status)
		printf("\x1B[31mFailed!\e[0m\n");
	else
	{
		dev_decnand->read_sector(0x0B130000 >> 9, 1, nand_sector);
		// TODO: lookup...
		strcpy(bootInfo.fw_ver1, "Unknown");
		strcpy(bootInfo.fw_ver2, "Unknown");
	}

	free(nand_sector);
}

static void loadSettings()
{
	const char *res_str[2] = {"\x1B[31mFailed!", "\x1B[32mOK!"};
	
	bool success = loadConfigFile();
	printf(res_str[success]);
}

u8 rng_get_byte()
{
	u32 tmp = REG_PRNG[0];
	for(u32 i = 8; i < 32; i += 8)
	{
		tmp ^= (u8)(tmp >> i);
	}
	return (u8)tmp;
}

static void loadFirmNand(void)
{
	dev_decnand->read_sector(0x0005A980, 0x00002000, (u8*)FIRM_LOAD_ADDR);
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

void clearConsoles()
{
	consoleSelect(&con_bottom);
	consoleClear();
	consoleSelect(&con_top);
	consoleClear();
}
