#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mem_map.h"
#include "types.h"
#include "util.h"
#include "pxi.h"
#include "hid.h"
#include "arm9/hardware.h"
#include "fatfs/ff.h"
#include "arm9/dev.h"
#include "arm9/fsutils.h"
#include "arm9/firm.h"
#include "arm9/config.h"
#include "arm9/timer.h"
#include "arm9/menu.h"
#include "arm9/main.h"

static void initWifiFlash(void);
static u32  mount_fs(void);
static void screen_init();
static void unit_detect();
static void boot_env_detect();
static void fw_detect();
static bool loadSettings(int *mode);
static void checkSetVerboseMode();

int main(void)
{
	int mode = BootModeNormal;

	hardwareInit();

	debugHashCodeRoData();
	
	uiInit();
	
	checkSetVerboseMode();
	
	consoleSelect(&con_bottom);
	
	uiPrintIfVerbose("\x1B[32mGood morning\nHello !\e[0m\n\n");
	
	PXI_sendWord(PXI_CMD_FORBID_POWER_OFF);
	
	uiPrintIfVerbose("Detecting unit...\n");

	unit_detect();

	initWifiFlash();

	uiPrintIfVerbose("Filesystem init...\n");
	
	/* Try to read the settings file ASAP. */
	if(mount_fs() > 0) // We got at least 1 drive mounted
	{
		uiPrintIfVerbose("Loading settings...\n");
	
		if(loadSettings(&mode))
		{
			uiPrintIfVerbose("Trying to launch FIRM from settings...\n");
		
			switch(mode)
			{
				case BootModeQuick:
					screen_init();
					uiDrawSplashScreen();
					/* fallthrough */
				case BootModeQuiet:
					tryLoadFirmwareFromSettings(false);
					if(isFirmLoaded())
						goto finish_firmlaunch;
					/* else fallthrough */
				case BootModeNormal:
					screen_init();
					break;
				default:
					panic();
			}
		}
		else screen_init();
	}
	
	if(mode != BootModeQuick)
		uiDrawSplashScreen();

	uiPrintIfVerbose("Detecting boot environment...\n");

	boot_env_detect();

	uiPrintIfVerbose("Detecting firmware...\n");

	fw_detect();

	uiPrintIfVerbose("Entering menu...\n");

	TIMER_sleep(600);

	consoleClear();

	enter_menu(MENU_STATE_MAIN);

	
finish_firmlaunch:

	uiPrintIfVerbose("Unmounting FS...\n");

	fsUnmountAll();
	
	uiPrintIfVerbose("Closing devices...\n");

	devs_close();

	hardwareDeinit();

	// TODO: Proper argc/v passing needs to be implemented.
	firm_launch(1, (const char**)(ITCM_KERNEL_MIRROR + 0x7470));

	return 0;
}

void devs_close()
{
	dev_sdcard->close();
	dev_decnand->close();
	dev_rawnand->close();
	dev_flash->close();
}

static void initWifiFlash(void)
{
	const char *const res_str[2] = {"\x1B[31m Failed!", "\x1B[32m OK!"};

	uiPrintIfVerbose(" Initializing Wifi flash...");
	bool flashRes = dev_flash->init();
	uiPrintIfVerbose("%s\x1B[0m\n", res_str[flashRes]);
	bootInfo.wififlash_status = flashRes;
}

static u32 mount_fs(void)
{
	FRESULT tmp;
	u32 res = 0;
	const char *const res_str[2] = {"\x1B[31m Failed!", "\x1B[32m OK!"};


	uiPrintIfVerbose(" Mounting SD card FAT FS...");
	tmp = f_mount(&sd_fs, "sdmc:", 1);
	bootInfo.sd_status = (tmp ? 0 : 2u);
	uiPrintIfVerbose("%s %d\x1B[0m\n", res_str[tmp == FR_OK], tmp);
	res |= (tmp ? 0 : 1u);


	uiPrintIfVerbose(" Mounting twln FS...");
	tmp = f_mount(&nand_twlnfs, "twln:", 1);
	uiPrintIfVerbose("%s %d\x1B[0m\n", res_str[tmp == FR_OK], tmp);
	res |= (tmp ? 0 : 1u<<1);

	uiPrintIfVerbose(" Mounting twlp FS...");
	tmp = f_mount(&nand_twlpfs, "twlp:", 1);
	uiPrintIfVerbose("%s %d\x1B[0m\n", res_str[tmp == FR_OK], tmp);
	res |= (tmp ? 0 : 1u<<2);

	uiPrintIfVerbose(" Mounting CTR NAND FS...");
	tmp = f_mount(&nand_fs, "nand:", 1);
	uiPrintIfVerbose("%s %d\x1B[0m\n", res_str[tmp == FR_OK], tmp);
	res |= (tmp ? 0 : 1u<<3);

	bootInfo.nand_status = res>>1;


	if(res != 0xFu && uiGetVerboseMode())
	{
		screen_init();
		TIMER_sleep(2000);
	}

	return res;
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
	bootInfo.unitIsNew3DS = REG_PDN_MPCORE_CFG & 2;

	sprintf(bootInfo.model, "%s 3DS", bootInfo.unitIsNew3DS ? "New" : "Original");
		
	uiPrintIfVerbose("%s detected!\n", bootInfo.model);
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
	
	strcpy(bootInfo.bootEnv, boot_environment[boot_env]);
}

static void fw_detect()
{
	u8 *nand_sector = (u8*)malloc(0x200);

	if(!bootInfo.nand_status)
		uiPrintIfVerbose("\x1B[31mFailed!\e[0m\n");
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
	uiPrintIfVerbose("%s\e[0m\n", res_str[success]);

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

static void checkSetVerboseMode()
{
	u32 keys;
	
	hidScanInput();
	keys = hidKeysDown() & HID_KEY_MASK_ALL;
	if(keys == HID_VERBOSE_MODE_BUTTONS)
		uiSetVerboseMode(true);
}

u8 rng_get_byte()
{
	u32 tmp = REG_PRNG[0];
	return (u8)tmp;
}

void power_off_safe()
{
	uiClearConsoles();

	uiPrintIfVerbose("Attempting to turn off...\n");

	fsUnmountAll();
	devs_close();
	// tell the arm11 we're done
	PXI_sendWord(PXI_CMD_POWER_OFF);

	for(;;);
}

void reboot_safe()
{
	wait(0x100000);
	
	uiClearConsoles();

	uiPrintIfVerbose("Attempting to reboot...\n");

	fsUnmountAll();
	devs_close();
	
	PXI_sendWord(PXI_CMD_REBOOT);

	for(;;);
}
