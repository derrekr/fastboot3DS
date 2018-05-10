/*
 *   This file is part of fastboot 3DS
 *   Copyright (C) 2017 derrek, profi200, d0k3
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

 
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// we need the ARM9 info from mem_map.h
#define ARM9
#include "mem_map.h"
#undef ARM9
#include "types.h"
#include "firmwriter.h"
#include "fs.h"
#include "fsutils.h"
#include "arm11/menu/battery.h"
#include "arm11/menu/bootslot.h"
#include "arm11/menu/menu.h"
#include "arm11/menu/menu_color.h"
#include "arm11/menu/menu_fsel.h"
#include "arm11/menu/menu_func.h"
#include "arm11/menu/menu_util.h"
#include "arm11/menu/splash.h"
#include "arm11/hardware/hid.h"
#include "arm11/hardware/mcu.h"
#include "arm11/console.h"
#include "arm11/config.h"
#include "arm11/debug.h"
#include "arm11/fmt.h"
#include "arm11/firm.h"



#define PRESET_SLOT_CONFIG_FUNC(x) \
u32 menuPresetSlotConfig##x(void) \
{ \
	return menuPresetSlotConfig((x-1)); \
}

u32 menuPresetNandTools(void)
{
	u32 res = 0xFF;
	
	if (!configDevModeEnabled())
		res &= ~((1 << 2) | (1 << 3)); // disable forced restore and firmware flash
	
	return res;
}

u32 menuPresetBootMenu(void)
{
	u32 res = 0xFF;
	
	for (u32 i = 0; i < N_BOOTSLOTS; i++)
	{
		if (!configDataExist(KBootOption1 + i))
		{
			res &= ~(1 << i);
		}
	}
	
	return res;
}

u32 menuPresetBootConfig(void)
{
	u32 res = 0;
	
	for (u32 i = 0; i < N_BOOTSLOTS; i++)
	{
		if (configDataExist(KBootOption1 + i))
		{
			res |= 1 << i;
		}
	}
	
	if (configDataExist(KBootMode))
		res |= 1 << N_BOOTSLOTS;
	
	if (configDataExist(KSplashScreen))
		res |= 1 << (N_BOOTSLOTS+1);
	
	if (configRamFirmBootEnabled())
		res |= 1 << (N_BOOTSLOTS+2);
	
	return res;
}

u32 menuPresetSplashConfig(void)
{
	return configDataExist(KSplashScreen) ? (1 << 0) : (1 << 1);
}

u32 menuPresetSlotConfig(u32 slot)
{
	u32 res = 0;
	
	if (configDataExist(KBootOption1 + slot))
	{
		res |= (1 << 0);
		res |= (configDataExist(KBootOption1Buttons + slot)) ? (1 << 1) : (1 << 2);
	}
	else
	{
		res |= (1 << 3);
	}
	
	return res;
}
PRESET_SLOT_CONFIG_FUNC(1)
PRESET_SLOT_CONFIG_FUNC(2)
PRESET_SLOT_CONFIG_FUNC(3)
PRESET_SLOT_CONFIG_FUNC(4)
PRESET_SLOT_CONFIG_FUNC(5)
PRESET_SLOT_CONFIG_FUNC(6)
PRESET_SLOT_CONFIG_FUNC(7)
PRESET_SLOT_CONFIG_FUNC(8)
PRESET_SLOT_CONFIG_FUNC(9)

u32 menuPresetBootMode(void)
{
	if (configDataExist(KBootMode))
	{
		return (1 << (*(u32*) configGetData(KBootMode)));
	}
		
	return 0;
}


u32 menuReturn(PrintConsole* term_con, PrintConsole* menu_con, u32 param)
{
	(void) term_con;
	(void) menu_con;
	return param;
}

u32 menuSetBootMode(PrintConsole* term_con, PrintConsole* menu_con, u32 param)
{
	(void) term_con;
	(void) menu_con;
	u32 res = (configSetKeyData(KBootMode, &param)) ? MENU_OK : MENU_FAIL;
	
	return res;
}

u32 menuSwitchFcramBoot(PrintConsole* term_con, PrintConsole* menu_con, u32 param)
{
	(void) term_con;
	(void) menu_con;
	(void) param;
	bool fcram_next = !configRamFirmBootEnabled(); 
	u32 res = (configSetKeyData(KRamFirmBoot, &fcram_next)) ? MENU_OK : MENU_FAIL;
	
	return res;
}

u32 menuSetSplash(PrintConsole* term_con, PrintConsole* menu_con, u32 param)
{
	char* res_path = NULL;
	char* start = NULL;
	
	// if (param == 0) set default splash screen and return to menu
	if (!param)
	{
		configDeleteKey(KSplashScreen);
		return MENU_OK;
	}
	
	if (configDataExist(KSplashScreen))
		start = (char*) configGetData(KSplashScreen);
	
	res_path = (char*) malloc(FF_MAX_LFN + 1);
	if (!res_path) panicMsg("Out of memory");
	
	consoleSelect(term_con);
	consoleClear();
	ee_printf_screen_center("Select a custom splash screen.\n[X] to select current folder.\nPress [HOME] to cancel.");
	updateScreens();
	
	u32 res = MENU_OK;
	*res_path = '\0';
	if (menuFileSelector(res_path, menu_con, start, "splash*.*", true, true))
	{
		// back to terminal console
		consoleSelect(term_con);
		
		// analyze user selection
		FILINFO fileStat;
		if ((fStat(res_path, &fileStat) == FR_OK) && !(fileStat.fattrib & AM_DIR))
		{
			char* slash = strrchr(res_path, '/');
			if (slash) *slash = '\0';
		}
		
		// check if selection at least looks valid
		const char* splash_name[] = { CSPLASH_NAME_TOP, CSPLASH_NAME_SUB };
		const u32 splash_bin_width[] = { SCREEN_WIDTH_TOP, SCREEN_WIDTH_SUB };
		const u32 splash_bin_height[] = { SCREEN_HEIGHT_TOP, SCREEN_HEIGHT_SUB };
		char* splash_path =  (char*) malloc(FF_MAX_LFN + 1);
		char* splash_bin_path =  (char*) malloc(FF_MAX_LFN + 1);
		bool valid = false;
		
		if (!splash_path || !splash_bin_path)
			panicMsg("Out of memory");
		
		for (u32 i = 0; i < 2; i++)
		{
			// check for splash in .spla format
			ee_snprintf(splash_path, FF_MAX_LFN + 1, "%s/%s.spla", res_path, splash_name[i]);
			if ((fStat(splash_path, &fileStat) == FR_OK) && (fileStat.fsize >= sizeof(SplashHeader)))
			{
				valid = true;
				continue;
			}
			
			// check splash in Luma 3DS .bin format
			u32 splash_bin_size = splash_bin_width[i] * splash_bin_height[i] * 3;
			ee_snprintf(splash_bin_path, FF_MAX_LFN + 1, "%s/%s.bin", res_path, splash_name[i]);
			if ((fStat(splash_bin_path, &fileStat) != FR_OK) || (fileStat.fsize != splash_bin_size))
				continue; // not found
			
			// notify user about the conversion
			consoleClear();
			ee_printf_screen_center("Converting splash files, please wait...");
			updateScreens();
			
			// convert .bin splash to .spla format
			s32 fHandle = fOpen(splash_bin_path, FS_OPEN_EXISTING | FS_OPEN_READ);
			if (fHandle < 0) continue; // can not open
			
			u8* splash_buffer = (u8*) malloc(splash_bin_size);
			if (!splash_buffer) panicMsg("Out of memory");
			
			fRead(fHandle, splash_buffer, splash_bin_size);
			fClose(fHandle);
			
			// convert RGB888 -> RGB565
			u8* ptr_out = splash_buffer;
			for (u8* ptr_in = splash_buffer;
				ptr_in - splash_buffer < (int) splash_bin_size;
				ptr_in += 3, ptr_out += 2)
			{
				u16 rgb565 =
					(ptr_in[2] >> 3) << (6 + 5) |
					(ptr_in[1] >> 2) << 5 |
					(ptr_in[0] >> 3);
				
				ptr_out[0] = rgb565 & 0xFF;
				ptr_out[1] = rgb565 >> 8; 
			}
			
			// build the .spla header
			SplashHeader hdr;
			memcpy(&(hdr.magic), "SPLA", 4);
			hdr.width = splash_bin_width[i];
			hdr.height = splash_bin_height[i];
			hdr.flags = FLAG_ROTATED;
			
			// write .spla file
			fHandle = fOpen(splash_path, FS_OPEN_ALWAYS | FS_OPEN_WRITE);
			if (fHandle < 0) { // cannot open for writing
				free(splash_buffer);
				continue;
			}
			
			valid = ((fWrite(fHandle, &hdr, sizeof(SplashHeader)) == FR_OK) &&
				(fWrite(fHandle, splash_buffer, hdr.width * hdr.height * 2) == FR_OK));
			fClose(fHandle);
			free(splash_buffer);
			
			if (!valid) break;
		}
		
		free(splash_path);
		free(splash_bin_path);
		
		if (valid)
		{
			res = (configSetKeyData(KSplashScreen, res_path)) ? MENU_OK : MENU_FAIL;
		}
		else
		{
			res = MENU_FAIL;
			
			consoleSelect(term_con);
			consoleClear();
			ee_printf_screen_center("Not a valid splash folder.\nPress [B] or [HOME] to return.");
			updateScreens();
			
			outputEndWait();
		}
	}
	
	free(res_path);
	return res;
}

u32 menuSetupBootSlot(PrintConsole* term_con, PrintConsole* menu_con, u32 param)
{
	const u32 slot = param & 0xF;
	char* res_path = NULL;
	char* start = NULL;
	
	// if bit4 of param is set, reset slot and return
	if (param & 0x10)
	{
		configDeleteKey(KBootOption1Buttons + slot);
		configDeleteKey(KBootOption1 + slot);
		return MENU_OK;
	}
	
	if (configDataExist(KBootOption1 + slot))
		start = (char*) configGetData(KBootOption1 + slot);
	
	res_path = (char*) malloc(FF_MAX_LFN + 1);
	if (!res_path) panicMsg("Out of memory");
	
	consoleSelect(term_con);
	consoleClear();
	ee_printf_screen_center("Select a firmware file for slot #%lu.\nPress [HOME] to cancel.", slot + 1);
	updateScreens();
	
	u32 res = MENU_OK;
	if (menuFileSelector(res_path, menu_con, start, "*firm*", true, false))
		res = (configSetKeyData(KBootOption1 + slot, res_path)) ? MENU_OK : MENU_FAIL;
	
	free(res_path);
	return res;
}

u32 menuSetupBootKeys(PrintConsole* term_con, PrintConsole* menu_con, u32 param)
{
	(void) menu_con;
	
	const u32 y_center = 7;
	const u32 y_instr = 12;
	const u32 slot = param & 0xF;
	
	// don't allow setting this up if firm is not set
	if (!configDataExist(KBootOption1 + slot))
		return MENU_OK;
	
	// if bit4 of param is set, delete boot keys and return
	if (param & 0x10)
	{
		configDeleteKey(KBootOption1Buttons + slot);
		return MENU_OK;
	}
	
	hidScanInput();
	u32 kHeld = hidKeysHeld();
	
	while (true)
	{
		// build button string
		char button_str[80];
		keysToString(kHeld, button_str);
		
		// clear console
		consoleSelect(term_con);
		consoleClear();
		
		// draw input block
		term_con->cursorY = y_center;
		ee_printf(ESC_SCHEME_WEAK);
		ee_printf_line_center("Hold button(s) to setup.");
		ee_printf_line_center("Currently held buttons:");
		ee_printf(ESC_SCHEME_ACCENT1);
		ee_printf_line_center(button_str);
		ee_printf(ESC_RESET);
		
		// draw instructions
		term_con->cursorY = y_instr;
		ee_printf(ESC_SCHEME_WEAK);
		if (configDataExist(KBootOption1Buttons + slot))
		{
			char* currentSetting =
				(char*) configCopyText(KBootOption1Buttons + slot);
			if (!currentSetting) panicMsg("Config error");
			ee_printf_line_center("Current: %s", currentSetting);
			free(currentSetting);
		}
		ee_printf_line_center("[HOME] to cancel");
		ee_printf(ESC_RESET);
		
		// update screens
		updateScreens();
		
		// check for buttons until held for ~1.5sec
		u32 kHeldNew = 0;
		do
		{
			// check hold duration
			u32 vBlanks = 0;
			do
			{
				GFX_waitForEvent(GFX_EVENT_PDC0, true);
				if(hidGetPowerButton(false)) return MENU_FAIL;
				
				hidScanInput();
				kHeldNew = hidKeysHeld();
				if(hidKeysDown() & KEY_SHELL) sleepmode();
			}
			while ((kHeld == kHeldNew) && (++vBlanks < 100));
			
			// check HOME key
			if (kHeldNew & KEY_HOME) return MENU_FAIL;
		}
		while (!((kHeld|kHeldNew) & 0xfff));
		// repeat checks until actual buttons are held
		
		if (kHeld == kHeldNew) break;
		kHeld = kHeldNew;
	}
	
	// if we arrive here, we have a button combo
	u32 res = (configSetKeyData(KBootOption1Buttons + slot, &kHeld)) ? MENU_OK : MENU_FAIL;
	
	return res;
}

u32 menuLaunchFirm(PrintConsole* term_con, PrintConsole* menu_con, u32 param)
{
	char path_store[FF_MAX_LFN + 1];
	char* path;
	
	// select & clear console
	consoleSelect(term_con);
	consoleClear();
		
	if (param < N_BOOTSLOTS) // loading from bootslot
	{
		// check if bootslot exists
		if (!configDataExist(KBootOption1 + param))
		{
			ee_printf("Bootslot does not exist!\n");
			goto fail;
		}
		path = (char*) configGetData(KBootOption1 + param);
	}
	else if (param == 0xFF) // user decision
	{
		ee_printf_screen_center("Select a firmware file to boot.\nPress [HOME] to cancel.");
		updateScreens();
		
		path = path_store;
		if (!menuFileSelector(path, menu_con, NULL, "*firm*", true, false))
			return MENU_FAIL;
		
		// back to terminal console
		consoleSelect(term_con);
		consoleClear();
	}
	
	// try load and verify
	ee_printf("\nLoading %s...\n", path);
	s32 res = loadVerifyFirm(path, false);
	if (res < 0)
	{
		ee_printf("Firm %s error code %li!\n", (res > -8) ? "load" : "verify", res);
		goto fail;
	}
	
	ee_printf("\nFirm load success, launching firm..."); // <-- you will never see this
	
	// store the bootslot
	u32 slot = (param < N_BOOTSLOTS) ? (param + 1) : 0;
	storeBootslot(slot);
	
	return (res == 1) ? MENU_RET_FIRMLOADED_SI : MENU_RET_FIRMLOADED;
	
	fail:
	
	ee_printf("\nFirm launcher failed.\n\nPress B or HOME to return.");
	updateScreens();
	outputEndWait();
	
	return MENU_FAIL;
}

u32 menuBackupNand(PrintConsole* term_con, PrintConsole* menu_con, u32 param)
{
	(void) menu_con;
	(void) param;
	s32 error = 0;
	u32 result = MENU_FAIL;
	
	// select & clear console
	consoleSelect(term_con);
	consoleClear();
	
	
	// ensure SD mounted
	if (!fsEnsureMounted("sdmc:"))
	{
		ee_printf("SD not inserted or corrupt!\n");
		goto fail;
	}
	
	// get NAND size (return value in sectors)
	const s64 nand_size = fGetDeviceSize(FS_DEVICE_NAND) * 0x200;
	if (!nand_size)
	{
		ee_printf("Failed communicating with NAND!\n");
		goto fail;
	}
	
	
	// console serial number
	char serial[0x10] = { 0 }; // serial from SecureInfo_?
	if (!fsQuickRead("nand:/rw/sys/SecureInfo_A", serial, 0xF, 0x102) && 
		!fsQuickRead("nand:/rw/sys/SecureInfo_B", serial, 0xF, 0x102))
		ee_snprintf(serial, 0x10, "UNKNOWN");
	
	// current state of the RTC
	u8 rtc[8] = { 0 };
	MCU_readRTC(rtc);
	
	// create NAND backup filename
	char fpath[64];
	ee_snprintf(fpath, 64, NAND_BACKUP_PATH "/%02X%02X%02X%02X%02X%02X_%s_nand.bin",
		rtc[6], rtc[5], rtc[4], rtc[2], rtc[1], rtc[0], serial);
	
	ee_printf(ESC_SCHEME_ACCENT1 "Creating NAND backup:\n%s\n" ESC_RESET "\nPreparing NAND backup...\n", fpath);
	updateScreens();
	
	
	// open file handle
	s32 fHandle;
	if (!fsCreateFileWithPath(fpath) ||
		((fHandle = fOpen(fpath, FS_OPEN_EXISTING | FS_OPEN_WRITE)) < 0))
	{
		ee_printf("Cannot create file!\n");
		goto fail;
	}
	
	// reserve space for NAND backup
	ee_printf("NAND size: %lli MiB\nBuffer size: %lu kiB\nReserving space...\n",
		nand_size / 0x0100000, (u32) DEVICE_BUFSIZE / 0x400);
	updateScreens();
	if ((fLseek(fHandle, nand_size) != 0) || (fTell(fHandle) != nand_size))
	{
		fClose(fHandle);
		fUnlink(fpath);
		ee_printf("Not enough space!\n");
		goto fail;
	}
	
	
	// setup device read
	s32 devHandle = fPrepareRawAccess(FS_DEVICE_NAND);
	if (devHandle < 0)
	{
		fClose(fHandle);
		fUnlink(fpath);
		ee_printf("Cannot open NAND device (error %li)!\n", devHandle);
		goto fail;
	}
	
	// setup device buffer
	s32 dbufHandle = fCreateDeviceBuffer(DEVICE_BUFSIZE);
	if (dbufHandle < 0)
		panicMsg("Out of memory");
	
	
	// all done, ready to do the NAND backup
	ee_printf("\n");
	for (s64 p = 0; p < nand_size; p += DEVICE_BUFSIZE)
	{
		s64 readBytes = (nand_size - p > DEVICE_BUFSIZE) ? DEVICE_BUFSIZE : nand_size - p;
		s32 errcode = 0;
		ee_printf_progress("NAND backup", PROGRESS_WIDTH, p, nand_size);
		updateScreens();
		
		if ((errcode = fReadToDeviceBuffer(devHandle, p, readBytes, dbufHandle)) != 0)
		{
			ee_printf("\nError: Cannot read from NAND (%li)!\n", errcode);
			goto fail_close_handles;
		}
		
		if ((errcode = fsWriteFromDeviceBuffer(fHandle, p, readBytes, dbufHandle)) != 0)
		{
			ee_printf("\nError: Cannot write to file (%li)!\n", errcode);
			goto fail_close_handles;
		}
		
		// check for user cancel request
		if (userCancelHandler(true))
		{
			fFinalizeRawAccess(devHandle);
			fFreeDeviceBuffer(dbufHandle);
			fClose(fHandle);
			fUnlink(fpath);
			return MENU_FAIL;
		}
	}
	
	// NAND access finalized
	ee_printf_progress("NAND backup", PROGRESS_WIDTH, nand_size, nand_size);
	ee_printf("\n" ESC_SCHEME_GOOD "NAND backup finished.\n" ESC_RESET);
	result = MENU_OK;
	
	
	fail_close_handles:
	
	if ((error = fFinalizeRawAccess(devHandle)))
		ee_printf("Failed closing NAND handle (error %li)!\n", error);
	fFreeDeviceBuffer(dbufHandle);
	fClose(fHandle);
	
	
	fail:
	
	ee_printf("\nPress B or HOME to return.");
	updateScreens();
	outputEndWait();

	
	if (result != MENU_OK) fUnlink(fpath);
	hidScanInput(); // throw away any input from impatient users
	return result;
}

u32 menuRestoreNand(PrintConsole* term_con, PrintConsole* menu_con, u32 param)
{
	bool forced = param; // if param != 0 -> forced restore
	s32 error = 0;
	u32 result = MENU_FAIL;
	
	
	// select & clear console
	consoleSelect(term_con);
	consoleClear();
	
	// check dev mode
	if (forced && !configDevModeEnabled()) {
		ee_printf("Forced restore is not available!\nEnable dev mode to get access.\n");
		goto fail;
	}
	
	// check battery
	BatteryState battery;
	getBatteryState(&battery);
	if ((battery.percent <= 20) && !battery.charging) {
		ee_printf("Battery below 20%% and not charging.\nPlug in the charger and retry.\n");
		goto fail;
	}
	
	// ensure SD mounted
	if (!fsEnsureMounted("sdmc:"))
	{
		ee_printf("SD not inserted or corrupt!\n");
		goto fail;
	}
	
	// get NAND size (return value in sectors)
	const s64 nand_size = fGetDeviceSize(FS_DEVICE_NAND) * 0x200;
	if (!nand_size)
	{
		ee_printf("Failed communicating with NAND!\n");
		goto fail;
	}
	
	
	ee_printf_screen_center("Select a NAND backup for restore.\nPress [HOME] to cancel.");
	updateScreens();
	
	char fpath[FF_MAX_LFN + 1];
	if (!menuFileSelector(fpath, menu_con, NAND_BACKUP_PATH, "*.bin", false, false))
		return MENU_FAIL; // canceled by user
	
	// select & clear console
	consoleSelect(term_con);
	consoleClear();
	
	// ask the user for confirmation
	if (forced)
	{
		if (!askConfirmation(ESC_SCHEME_BAD "WARNING:" ESC_RESET "\nYou're about to force-restore a NAND image to\nyour system. Doing this with an incompatible\nNAND image will **BRICK** your console! Make\nsure you backed up your important data!")) return MENU_FAIL;
	}
	else
	{
		if (!askConfirmation(ESC_SCHEME_BAD "WARNING:" ESC_RESET "\nYou're about to restore a NAND image to\nyour system. Make sure you have backups of\nyour important data!")) return MENU_FAIL; 
	}
	consoleClear();
	
	// check NAND backup (when not forced)
	if (!forced && (fVerifyNandImage(fpath) != 0))
	{
		ee_printf("%s\nNot a valid NAND backup for this 3DS!\n", fpath);
		goto fail;
	}
	
	ee_printf(ESC_SCHEME_ACCENT1 "Restoring NAND backup:\n%s\n" ESC_RESET "\nPreparing NAND restore...\n", fpath);
	updateScreens();
	
	
	// open file handle
	s32 fHandle;
	if ((fHandle = fOpen(fpath, FS_OPEN_EXISTING | FS_OPEN_READ)) < 0)
	{
		ee_printf("Cannot open file (error %li)!\n", fHandle);
		goto fail;
	}
	
	// setup device read
	s32 devHandle = fPrepareRawAccess(FS_DEVICE_NAND);
	if (devHandle < 0)
	{
		fClose(fHandle);
		fUnlink(fpath);
		ee_printf("Cannot open NAND device (error %li)!\n", devHandle);
		goto fail;
	}
	
	// setup device buffer
	s32 dbufHandle = fCreateDeviceBuffer(DEVICE_BUFSIZE);
	if (dbufHandle < 0)
		panicMsg("Out of memory");
	
	
	// check file size
	const s64 file_size = fSize(fHandle);
	ee_printf("File size: %lli MiB\n", file_size / 0x100000);
	ee_printf("NAND size: %lli MiB\n", nand_size / 0x100000);
	ee_printf("Buffer size: %lu kiB\n", (u32) DEVICE_BUFSIZE / 0x400);
	updateScreens();
	if (file_size > nand_size)
	{
		ee_printf("Size exceeds available space!\n");
		goto fail_close_handles;
	}
	
	
	// setup NAND protection
	bool protected = !forced;
	if (fSetNandProtection(protected) != 0)
		panicMsg("Set NAND protection failed.");
	ee_printf("NAND protection: %s\n", protected ? "enabled" : "disabled");
	
	
	// all done, ready to do the NAND backup
	ee_printf("\n");
	for (s64 p = 0; p < file_size; p += DEVICE_BUFSIZE)
	{
		s64 readBytes = (file_size - p > DEVICE_BUFSIZE) ? DEVICE_BUFSIZE : file_size - p;
		s32 errcode = 0;
		ee_printf_progress("NAND restore", PROGRESS_WIDTH, p, file_size);
		updateScreens();
		
		if ((errcode = fReadToDeviceBuffer(fHandle, p, readBytes, dbufHandle)) != 0)
		{
			ee_printf("\nError: Cannot read from file (%li)!\n", errcode);
			goto fail_close_handles;
		}
		
		if ((errcode = fsWriteFromDeviceBuffer(devHandle, p, readBytes, dbufHandle)) != 0)
		{
			ee_printf("\nError: Cannot write to NAND (%li)!\n", errcode);
			goto fail_close_handles;
		}
		
		// check for user cancel request
		// cancel is forbidden(!) here, but we need to handle force poweroff
		if (userCancelHandler(false))
		{
			fFinalizeRawAccess(devHandle);
			fFreeDeviceBuffer(dbufHandle);
			fClose(fHandle);
			return MENU_FAIL;
		}
	}
	
	// NAND access finalized
	ee_printf_progress("NAND restore", PROGRESS_WIDTH, file_size, file_size);
	ee_printf("\n" ESC_SCHEME_GOOD "NAND restore finished.\n" ESC_RESET);
	result = MENU_OK;
	
	
	fail_close_handles:

	if ((error = fFinalizeRawAccess(devHandle)))
		ee_printf("Failed closing NAND handle (error %li)!\n", error);
	fFreeDeviceBuffer(dbufHandle);
	fClose(fHandle);
	
	
	fail:
	
	ee_printf("\nPress B or HOME to return.");
	updateScreens();
	outputEndWait();

	
	return result;
}

u32 menuInstallFirm(PrintConsole* term_con, PrintConsole* menu_con, u32 param)
{
	char firm_drv[8] = { 'f', 'i', 'r', 'm', '0' + param, ':', '\0' };
	char firm_path[FF_MAX_LFN + 1];
	u32 result = MENU_FAIL;
	
	
	// clear console
	consoleSelect(term_con);
	consoleClear();
	
	// check dev mode
	if (!configDevModeEnabled()) {
		ee_printf("Install firmware is not available!\nEnable dev mode to get access.\n");
		goto fail;
	}
	
	// file selector
	ee_printf_screen_center("Select a firmware file to install.\nPress [HOME] to cancel.");
	updateScreens();
	if (!menuFileSelector(firm_path, menu_con, NULL, "*firm*", true, false))
		return MENU_FAIL; // cancel by user
	
	
	// select and clear console
	consoleSelect(term_con);
	consoleClear();
	
	// ask the user for confirmation
	if (!askConfirmation(ESC_SCHEME_BAD "WARNING:" ESC_RESET "\nYou're about to install a firmware to %s.\nFlashing incompatible firmwares may lead to\nunexpected results.", firm_drv)) return MENU_FAIL;
	consoleClear();
	
	ee_printf(ESC_SCHEME_ACCENT1 "Flashing firmware to %s:\n%s\n" ESC_RESET "\nLoading firmware... ", firm_drv, firm_path);
	updateScreens();
	
	s32 res = loadVerifyFirm(firm_path, false);
	if (res < 0)
	{
		ee_printf(ESC_SCHEME_BAD "failed!\n" ESC_RESET);
		ee_printf("Firm %s error code %li!\n", (res > -8) ? "load" : "verify", res);
		goto fail;
	}
	
	ee_printf(ESC_SCHEME_GOOD "OK\n" ESC_RESET "Flashing firmware... ");
	updateScreens();
	
	res = writeFirmPartition(firm_drv, true);
	if (res != 0)
	{
		ee_printf(ESC_SCHEME_BAD "failed!\n" ESC_RESET);
		ee_printf("Firm flash error code %li!\n", res);
		goto fail;
	}
	
	ee_printf(ESC_SCHEME_GOOD "OK\n" ESC_RESET);
	ee_printf(ESC_SCHEME_GOOD "\nFirm was flashed to %s.\n" ESC_RESET, firm_drv);
	result = MENU_OK;
	
	
	fail:
	
	ee_printf("\nPress B or HOME to return.");
	updateScreens();
	outputEndWait();

	
	return result;
}

u32 menuDumpBootrom(PrintConsole* term_con, PrintConsole* menu_con, u32 param)
{
	extern const bool __superhaxEnabled;

	(void) menu_con;
	(void) param;

	u32 result = MENU_FAIL;

	// bootrom dumper output
	consoleSelect(term_con);
	consoleClear();
	ee_printf(ESC_SCHEME_ACCENT1 "Dumping Bootroms and OTP...\n\n" ESC_RESET);

	// if superhax is not enabled: enable it and reboot
	// (carefull not to introduce a potential bootloop here!)
	if (!__superhaxEnabled)
	{
		ee_printf("Enabling SuperHax... ");
		s32 ret = toggleSuperhax(true);
		if (ret != 0)
		{
			ee_printf(ESC_SCHEME_BAD "failed!\n" ESC_RESET);
			if (ret == -6)
				ee_printf("Fastboot3DS not installed in FIRM0.\n");
			else
				ee_printf("Unknown error.\n");
			goto fail;
		}
		else
		{
			ee_printf(ESC_SCHEME_GOOD "success\n" ESC_RESET);
			ee_printf("Now rebooting...");
			return MENU_RET_REBOOT;
		}
	}

	// if we arrive here: superhax enabled, ready to dump
	bool valid;
	s32 kDown;

	// dump boot9.bin
	ee_printf("Dumping ARM9 bootrom... ");
	valid = fsQuickCreate("sdmc:/3DS/boot9.bin", (void*) BOOT9_BASE, BOOT9_SIZE);
	ee_printf(valid ? ESC_SCHEME_GOOD "success\n" ESC_RESET : ESC_SCHEME_BAD "failed!\n" ESC_RESET);
	updateScreens();

	// dump boot11.bin
	ee_printf("Dumping ARM11 bootrom... ");
	valid = fsQuickCreate("sdmc:/3DS/boot11.bin", (void*) (VRAM_BASE + VRAM_SIZE - BOOT11_SIZE), BOOT11_SIZE);
	ee_printf(valid ? ESC_SCHEME_GOOD "success\n" ESC_RESET : ESC_SCHEME_BAD "failed!\n" ESC_RESET);
	updateScreens();

	// dump otp.bin
	ee_printf("Dumping OTP... ");
	valid = fsQuickCreate("sdmc:/3DS/otp.bin", (void*) OTP_BASE, OTP_SIZE);
	ee_printf(valid ? ESC_SCHEME_GOOD "success\n" ESC_RESET : ESC_SCHEME_BAD "failed!\n" ESC_RESET);
	updateScreens();

	// disable superhax
	ee_printf("Disabling SuperHax... ");
	if (toggleSuperhax(false) != 0)
	{
		ee_printf(ESC_SCHEME_BAD "failed!\n" ESC_RESET);
	}
	else
	{
		ee_printf(ESC_SCHEME_GOOD "success\n" ESC_RESET);
		result = MENU_RET_POWEROFF;
	}


	fail:
	
	ee_printf("\nPress B or HOME to %s.", (result == MENU_RET_POWEROFF) ? "power off" : "return");
	updateScreens();
	outputEndWait();

	
	return result;
}

u32 menuUpdateFastboot3ds(PrintConsole* term_con, PrintConsole* menu_con, u32 param)
{
	(void) param;
	
	char firm_path[FF_MAX_LFN + 1];
	u32 result = MENU_FAIL;
	
	bool accept_downgrades = false;
	if (configDevModeEnabled())
		accept_downgrades = true;
	
	
	// file browser dialogue
	consoleSelect(term_con);
	consoleClear();
	
	ee_printf_screen_center("Select fastboot3DS update file.\nPress [HOME] to cancel.");
	updateScreens();
	if (!menuFileSelector(firm_path, menu_con, "sdmc:", "*firm*", true, false))
		return MENU_FAIL; // cancel by user
	
	
	// verify and install update
	consoleSelect(term_con);
	consoleClear();
	
	ee_printf(ESC_SCHEME_ACCENT1 "Updating fastboot3DS from file:\n%s\n" ESC_RESET "\nChecking battery... ", firm_path);
	
	BatteryState battery;
	getBatteryState(&battery);
	if ((battery.percent <= 5) && !battery.charging) {
		ee_printf(ESC_SCHEME_BAD "low!\n" ESC_RESET);
		ee_printf("Battery below 5%% and not charging.\nPlug in the charger and retry.\n");
		goto fail;
	} else ee_printf(ESC_SCHEME_GOOD "ok\n" ESC_RESET);

	ee_printf("Loading firmware... ");
	updateScreens();
	
	u32 version = 0;
	s32 res = loadVerifyUpdate(firm_path, &version);
	if (!accept_downgrades && (res == UPDATE_ERR_DOWNGRADE))
	{
		ee_printf(ESC_SCHEME_BAD "failed!\n" ESC_RESET);
		ee_printf("A newer version is already installed.\n");
		goto fail;
	}
	else if (res != 0)
	{
		ee_printf(ESC_SCHEME_BAD "failed!\n" ESC_RESET);
		switch ( res )
		{
			case UPDATE_ERR_INVALID_FIRM:
				ee_printf("Firm validation failed.\n");
				break;
				
			case UPDATE_ERR_INVALID_SIG:
				ee_printf("Not a fastboot3DS update firmware.\n");
				break;
				
			case UPDATE_ERR_NOT_INSTALLED:
				ee_printf("Update is not possible.\n");
				break;
				
			default:
				ee_printf("Update error code %li!\n", res);
				break;
		}
		goto fail;
	}
	
	ee_printf(ESC_SCHEME_GOOD "v%lu.%lu\n" ESC_RESET "Flashing firmware... ", (version >> 16) & 0xFFFF, version & 0xFFFF);
	updateScreens();
	
	res = writeFirmPartition("firm0:", true);
	if (res != 0)
	{
		ee_printf(ESC_SCHEME_BAD "failed!\n" ESC_RESET);
		ee_printf("Firm flash error code %li!\n", res);
		goto fail;
	}
	
	ee_printf(ESC_SCHEME_GOOD "OK\n" ESC_RESET);
	ee_printf(ESC_SCHEME_GOOD "\nfastboot3DS was updated.\nSystem will reboot.\n" ESC_RESET);
	result = MENU_RET_REBOOT;
	
	
	fail:
	
	ee_printf("\nPress B or HOME to return.");
	updateScreens();
	outputEndWait();

	
	return result;
}

u32 menuShowCredits(PrintConsole* term_con, PrintConsole* menu_con, u32 param)
{
	(void) menu_con;
	(void) param;
	
	// clear console
	consoleSelect(term_con);
	consoleClear();
	
	// credits
	term_con->cursorY = 1;
	ee_printf(ESC_SCHEME_ACCENT0);
	ee_printf_line_center("Fastboot3DS Credits");
	ee_printf_line_center("===================");
	ee_printf_line_center("");
	ee_printf(ESC_SCHEME_STD);
	ee_printf_line_center("Main developers:");
	ee_printf(ESC_SCHEME_WEAK);
	ee_printf_line_center("derrek");
	ee_printf_line_center("profi200");
	ee_printf_line_center("d0k3");
	ee_printf_line_center("");
	ee_printf(ESC_SCHEME_STD);
	ee_printf_line_center("Thanks to:");
	ee_printf(ESC_SCHEME_WEAK);
	ee_printf_line_center("yellows8");
	ee_printf_line_center("plutoo");
	ee_printf_line_center("smea");
	ee_printf_line_center("Normmatt (for sdmmc code)");
	ee_printf_line_center("WinterMute (for console code)");
	ee_printf_line_center("ctrulib devs (for HID code)");
	ee_printf_line_center("Luma 3DS devs (for fmt.c/gfx code)");
	ee_printf_line_center("mtheall (for LZ11 decompress code)");
	ee_printf_line_center("devkitPro (for the toolchain/makefiles)");
	ee_printf_line_center("ChaN (for the FATFS library)");
	ee_printf_line_center("... everyone who contributed to 3dbrew.org");
	updateScreens();

	
	// Konami code
	const u32 konami_code[] = {
		KEY_DUP, KEY_DUP, KEY_DDOWN, KEY_DDOWN, KEY_DLEFT, KEY_DRIGHT, KEY_DLEFT, KEY_DRIGHT, KEY_B, KEY_A };
	const u32 konami = sizeof(konami_code) / sizeof(u32);
	u32 k = 0;
	
	// handle user input
	u32 kDown = 0;
	do
	{
		GFX_waitForEvent(GFX_EVENT_PDC0, true);
		
		if(hidGetPowerButton(false)) // handle power button
			break;
		
		hidScanInput();
		kDown = hidKeysDown();
		
		if (kDown) k = (kDown & konami_code[k]) ? k + 1 : 0;
		if (!k && (kDown & KEY_B)) break;
		if (kDown & KEY_SHELL) sleepmode();
	}
	while (!(kDown & KEY_HOME) && (k < konami));
	
	
	// Konami code entered?
	if (k == konami)
	{
		const bool enabled = true;
		configSetKeyData(KDevMode, &enabled);
		
		consoleClear();
		term_con->cursorY = 9;
		ee_printf(ESC_SCHEME_ACCENT1);
		ee_printf_line_center("You are now a developer!");
		ee_printf(ESC_RESET);
		ee_printf_line_center("");
		ee_printf_line_center("Access to developer-only features granted.");
		updateScreens();
		
		outputEndWait();
	}
	
	
	return MENU_OK;
}

/*
u32 menuDummyFunc(PrintConsole* term_con, PrintConsole* menu_con, u32 param)
{
	(void) menu_con;
	
	// clear console
	consoleSelect(term_con);
	consoleClear();
	
	// print something
	ee_printf("This is not implemented yet.\nMy parameter was %lu.\nGo look elsewhere, nothing to see here.\n\nPress B or HOME to return.", param);
	updateScreens();
	outputEndWait();

	return MENU_OK;
}

u32 debugSettingsView(PrintConsole* term_con, PrintConsole* menu_con, u32 param)
{
	(void) menu_con;
	(void) param;
	
	// clear console
	consoleSelect(term_con);
	consoleClear();
	
	ee_printf("Config has changed: %s\n", configHasChanged() ? "true" : "false");
	ee_printf("Write config: %s\n", writeConfigFile() ? "success" : "failed");
	ee_printf("Load config: %s\n", loadConfigFile() ? "success" : "failed");
	
	// show settings
	for (int key = 0; key < KLast; key++)
	{
		const char* kText = configGetKeyText(key);
		const bool kExist = configDataExist(key);
		ee_printf("%02i %s: %s\n", key, kText, kExist ? "exists" : "not found");
		if (configDataExist(key))
		{
			char* text = (char*) configCopyText(key);
			ee_printf("text: %s / u32: %lu\n", text, *(u32*) configGetData(key));
			free(text);
		}
	}
	updateScreens();
	
	// wait for B / HOME button
	do
	{
		GFX_waitForEvent(GFX_EVENT_PDC0, true);
		if(hidGetPowerButton(false)) // handle power button
			return 0;
		
		hidScanInput();
	}
	while (!(hidKeysDown() & (KEY_B|KEY_HOME)));
	
	return 0;
}

u32 debugEscapeTest(PrintConsole* term_con, PrintConsole* menu_con, u32 param)
{
	(void) menu_con;
	(void) param;
	
	// clear console
	consoleSelect(term_con);
	consoleClear();
	
	ee_printf("\x1b[1mbold\n\x1b[0m");
	ee_printf("\x1b[2mfaint\n\x1b[0m");
	ee_printf("\x1b[3mitalic\n\x1b[0m");
	ee_printf("\x1b[4munderline\n\x1b[0m");
	ee_printf("\x1b[5mblink slow\n\x1b[0m");
	ee_printf("\x1b[6mblink fast\n\x1b[0m");
	ee_printf("\x1b[7mreverse\n\x1b[0m");
	ee_printf("\x1b[8mconceal\n\x1b[0m");
	ee_printf("\x1b[9mcrossed-out\n\x1b[0m");
	ee_printf("\n");
	
	for (u32 i = 0; i < 8; i++)
	{
		char c[8];
		ee_snprintf(c, 8, "\x1b[%lu", 30 + i);
		ee_printf("color #%lu:  %smnormal\x1b[0m %s;2mfaint\x1b[0m %s;4munderline\x1b[0m %s;7mreverse\x1b[0m %s;9mcrossed-out\x1b[0m\n", i, c, c, c, c, c);
	}
	
	// wait for B / HOME button
	do
	{
		updateScreens();
		if(hidGetPowerButton(false)) // handle power button
			return 0;
		
		hidScanInput();
	}
	while (!(hidKeysDown() & (KEY_B|KEY_HOME)));
	
	return 0;
}
*/
