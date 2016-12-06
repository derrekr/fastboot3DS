#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "pxi.h"
#include "hid.h"
#include "arm9/loader_init.h"
#include "fatfs/ff.h"
#include "arm9/dev.h"
#include "arm9/firm.h"
#include "arm9/config.h"
#include "arm9/timer.h"
#include "arm9/menu.h"
#include "arm9/main.h"

static bool devs_init();
static bool mount_fs();
static void screen_init();
static void unit_detect();
static void boot_env_detect();
static void fw_detect();
static bool loadSettings(int *mode);
bool tryLoadFirmwareFromSettings();
u32 loadFirmSd(const char *filePath);

// TODO: remove after debugging
extern void panic(void);
extern void hashCodeRoData(void);

int main(void)
{
	int mode;
	bool firmLoaded = false;

	hardwareInit();

	hashCodeRoData();	// TODO: remove after debugging

	PXI_sendWord(PXI_CMD_FORBID_POWER_OFF);

	//Initialize console for both screens using the two different PrintConsole we have defined
	consoleInit(SCREEN_TOP, &con_top);
	consoleSetWindow(&con_top, 1, 1, con_top.windowWidth - 2, con_top.windowHeight - 2);
	consoleInit(SCREEN_LOW, &con_bottom);
	
	consoleSelect(&con_top);
	
	printf("\x1B[32mGood morning\nHello !\e[0m\n\n");
	
	printf("Initializing devices...\n");

	if(!devs_init()) screen_init();

	printf("Mounting filesystems...\n");

	if(!mount_fs()) screen_init();

	printf("Loading settings...\n");

	if(!loadSettings(&mode)) screen_init();

	switch(mode)
	{
		case BootModeQuick:
			screen_init();
		case BootModeQuiet:
			firmLoaded = tryLoadFirmwareFromSettings();
			if(firmLoaded)
				break;
		case BootModeNormal:
			screen_init();
			break;
		default:
			panic();
	}

	if(!firmLoaded)
	{
		printf("Detecting unit...\n");

		unit_detect();

		printf("Detecting boot environment...\n");

		boot_env_detect();

		printf("Detecting firmware...\n");

		fw_detect();

		printf("Entering menu...\n");

		TIMER_sleep(2000);

		consoleClear();

		enter_menu(MENU_STATE_MAIN);
	}
	
	printf("Unmounting FS...\n");

	unmount_fs();
	
	printf("Closing devices...\n");

	devs_close();

	return 0;
}

static bool devs_init()
{
	const char *res_str[2] = {"\x1B[31mFailed!", "\x1B[32mOK!"};
	bool sdRes, nandRes, wifiRes;

	printf(" Initializing SD card... ");
	printf("%s\e[0m\n", res_str[sdRes = dev_sdcard->init()]);
	printf(" Initializing raw NAND... ");
	printf("%s\e[0m\n", res_str[nandRes = dev_rawnand->init()]);
	printf(" Initializing Wifi flash... ");
	printf("%s\e[0m\n", res_str[wifiRes = dev_flash->init()]);

	bootInfo.sd_status = sdRes;
	bootInfo.nand_status = nandRes;
	bootInfo.wififlash_status = wifiRes;

	// atm only those boot options are possible, so if the init
	// for both failed, this is a fatal error.
	return sdRes && nandRes;
}

void devs_close()
{
	dev_sdcard->close();
	dev_decnand->close();
	dev_rawnand->close();
	dev_flash->close();
}

bool remount_nand_fs()
{
	bool res = true;

	res &= f_mount(&nand_twlnfs, "twln:", 1);
	res &= f_mount(&nand_twlpfs, "twlp:", 1);
	res &= f_mount(&nand_fs, "nand:", 1);

	return res;
}

void unmount_nand_fs()
{
	f_mount(NULL, "twln:", 1);
	f_mount(NULL, "twlp:", 1);
	f_mount(NULL, "nand:", 1);
}

static bool mount_fs()
{
	FRESULT res[4];
	bool finalRes = true;
	const char *res_str[2] = {"\x1B[31mFailed!", "\x1B[32mOK!"};

	if(bootInfo.sd_status)
	{
		printf("Mounting SD card FAT FS... ");
		res[0] = f_mount(&sd_fs, "sdmc:", 1);
		if(res[0] == FR_OK) printf("%s\e[0m\n", res_str[1]);
		else printf("%s ERROR 0x%d\e[0m\n", res_str[0], res[0]);
		finalRes &= res[0] == FR_OK;
	}
	else finalRes = false;

	if(bootInfo.nand_status)
	{
		printf("Mounting twln FS... ");
		res[1] = f_mount(&nand_twlnfs, "twln:", 1);
		if(res[1] == FR_OK) printf("%s\e[0m\n", res_str[1]);
		else printf("%s ERROR 0x%d\e[0m\n", res_str[0], res[1]);

		printf("Mounting twlp FS... ");
		res[2] = f_mount(&nand_twlpfs, "twlp:", 1);
		if(res[2] == FR_OK) printf("%s\e[0m\n", res_str[1]);
		else printf("%s ERROR 0x%d\e[0m\n", res_str[0], res[2]);

		printf("Mounting CTR NAND FAT FS... ");
		res[3] = f_mount(&nand_fs, "nand:", 1);
		if(res[3] == FR_OK) printf("%s\e[0m\n", res_str[1]);
		else printf("%s ERROR 0x%d\e[0m\n", res_str[0], res[3]);

		finalRes &= res[3] == res[2] == res[1] == FR_OK;
	}
	else finalRes = false;

	if(!finalRes)
		TIMER_sleep(2000);

	return finalRes;
}

void unmount_fs()
{
	f_mount(NULL, "sdmc:", 1);
	f_mount(NULL, "twln:", 1);
	f_mount(NULL, "twlp:", 1);
	f_mount(NULL, "nand:", 1);
}

static void screen_init()
{
	static bool screenInitDone;
	if(!screenInitDone)
	{
		screenInitDone = true;
		PXI_sendWord(PXI_CMD_ENABLE_LCDS);
		while(PXI_recvWord() != PXI_RPL_OK);
	}
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

static bool loadSettings(int *mode)
{
	const char *res_str[2] = {"\x1B[31mFailed!", "\x1B[32mOK!"};
	
	bool success = loadConfigFile();
	printf("%s\e[0m\n", res_str[success]);

	if(success)
	{
		const int *mode_ptr = (const int *)configGetData(KBootMode);
		if(mode_ptr != NULL)
			 *mode = *mode_ptr;
		else *mode = BootModeNormal;
		return true;
	}

	*mode = BootModeNormal;

	return false;
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

bool tryLoadFirmwareFromSettings(void)
{
	const char *path;
	int keyBootOption, keyPad;
	int i;
	u32 padValue, expectedPadValue;

	consoleSelect(&con_top);

	printf("\n\nAttempting to load FIRM from settings...\n");

	keyBootOption = KBootOption1;
	keyPad = KBootOption1Buttons;

	for(i=0; i<3; i++, keyBootOption++, keyPad++)
	{
		path = (const char *)configGetData(keyBootOption);
		if(path)
		{
			printf("Boot Option #%i:\n", i + 1);
			// check pad value
			const u32 *temp;
			temp = (const u32 *)configGetData(keyPad);
			if(temp)
			{
				expectedPadValue = *temp;
				hidScanInput();
				padValue = HID_KEY_MASK_ALL & hidKeysHeld();
				if(padValue != expectedPadValue)
				{
					printf("Skipping, right buttons are not pressed.\n");
					printf("%x %x\n", padValue, expectedPadValue);
					continue;
				}
			}

			if(tryLoadFirmware(path))
				break;
		}

		// ... we failed, try next one
	}

	if(i >= 3)
		return false;

	return true;
}

bool tryLoadFirmware(const char *filepath)
{
	u32 fw_size;

	if(!filepath)
		return false;

	printf("Loading firmware:\n%s\n", filepath);

	if(strncmp(filepath, "sdmc:", 5) == 0)
		fw_size = loadFirmSd(filepath);
	else
		return false;	// TODO: Support more devices

	if(fw_size == 0)
		return false;

	return firm_verify(fw_size);
}

static void loadFirmNand(void)
{
	dev_decnand->read_sector(0x0005A980, 0x00002000, (u8*)FIRM_LOAD_ADDR);
}

u32 loadFirmSd(const char *filePath)
{
	FIL file;
	u32 fileSize;
	UINT bytesRead = 0;
	FILINFO fileStat;

	if(f_stat(filePath, &fileStat) != FR_OK)
	{
		printf("Failed to get file status!\n");
		return 0;
	}

	fileSize = fileStat.fsize;

	if(f_open(&file, filePath, FA_READ) != FR_OK)
	{
		printf("Failed to open '%s'!\n", filePath);
		return 0;
	}

	if(fileSize > FIRM_MAX_SIZE)
	{
		f_close(&file);
		return 0;
	}

	if(f_read(&file, (u8*)FIRM_LOAD_ADDR, fileSize, &bytesRead) != FR_OK)
	{
		printf("Failed to read from file!\n");
		fileSize = 0;
	}

	if(bytesRead != fileSize)
		fileSize = 0;

	f_close(&file);

	return fileSize;
}

void clearConsoles()
{
	consoleSelect(&con_bottom);
	consoleClear();
	consoleSelect(&con_top);
	consoleClear();
}

void power_off_safe()
{
	clearConsoles();

	printf("Attempting to turn off...\n");

	unmount_fs();
	devs_close();
	// tell the arm11 we're done
	PXI_sendWord(PXI_CMD_POWER_OFF);

	for(;;);
}
