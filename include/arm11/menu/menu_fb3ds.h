#pragma once

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

#include "types.h"
#include "arm11/menu/bootslot.h"
#include "arm11/menu/menu.h"
#include "arm11/menu/menu_func.h"

#define SUBENTRY_SLOT_BOOT(x) \
	{ "Boot [slot " #x "]",				DESC_BOOT_SLOT(x),			&menuLaunchFirm,		(x-1) }
	
#define SUBENTRY_SLOT_SETUP(x, m) \
	{ "Setup [slot " #x "]...",			DESC_SLOT_SETUP(x),			NULL,					(x+m-1) }
	
#define SUBMENU_SLOT_SETUP(x) \
{ \
	"Boot Slot #" #x " Setup", 4, &menuPresetSlotConfig##x, MENU_FLAG_SLOT(x) | MENU_FLAG_CONFIG, \
	{ \
		{ "Select [slot " #x "] firm",		DESC_FIRM_SLOT(x),			&menuSetupBootSlot,		(x-1)&0xF }, \
		{ "Set [slot " #x "] keycombo",		DESC_KEYS_SLOT(x),			&menuSetupBootKeys,		(x-1)&0xF }, \
		{ "Set [slot " #x "] autoboot",		DESC_AUTO_SLOT(x),			&menuSetupBootKeys,		0x10|((x-1)&0xF) }, \
		{ "Disable [slot " #x "]",			DESC_CLEAR_SLOT(x),			&menuSetupBootSlot,		0x10|((x-1)&0xF) } \
	} \
}

#define DESC_CONTINUE		"Continue booting the first available boot slot.\nNo function if boot slots are not set up."
#define DESC_BOOT_MENU		"Display a boot menu, allowing you to select which boot slot to boot from. Also includes boot slot and boot mode setup."
#define DESC_BOOT_FILE		"Select a firmware file to boot."
#define DESC_NAND_TOOLS		"Enter NAND tools submenu, including tools for NAND backup, NAND restore and firmware flash."
#define DESC_OPTIONS		"Enter fastboot3ds settings submenu."
#define DESC_MISC			"Enter miscellaneous submenu, including the update tool and credits section."

#define DESC_BOOT_SLOT(x)	"Boot the firmware in slot #" #x "."
#define DESC_BOOT_SETUP     "Change boot settings."

#define DESC_SLOT_SETUP(x)	"Change boot settings for slot #" #x "."
#define DESC_FIRM_SLOT(x)	"Change the firmware in boot slot #" #x "."
#define DESC_KEYS_SLOT(x)	"Disable autoboot and set a keycombo\nfor boot slot #" #x "."
#define DESC_AUTO_SLOT(x)	"Enable autoboot for boot slot #" #x ".  "
#define DESC_CLEAR_SLOT(x)	"Reset and disable boot slot #" #x ".    "

#define DESC_BOOT_NORMAL	"In normal boot mode, you will be presented with the fastboot3ds menu upon boot."
#define DESC_BOOT_QUICK		"In quick boot mode, splash is displayed and the boot is continued via the first available autoboot slot.\n \n! To enter the menu, hold the HOME button at boot !"
#define DESC_BOOT_QUIET		"In quiet boot mode, splash is not displayed and the boot is continued via the first available autoboot slot.\n \n! To enter the menu, hold the HOME button at boot !"
#define DESC_CHANGE_BOOT	"Change fastboot3ds boot mode. This allows you to set up how your console boots."

#define DESC_NAND_BACKUP	"Backup current NAND to a file."
#define DESC_NAND_RESTORE	"Restore current NAND from a file.\nThis option preserves your fastboot3ds installation."
#define DESC_NAND_RESTORE_F	"Restore current NAND from a file.\nWARNING: This will overwrite all of your flash memory, also overwriting fastboot3ds."
#define DESC_FIRM_FLASH		"Flash firmware from file to firm1:.\nWARNING: This will allow you to flash unsigned firmware, overwriting anything previously installed in firm1:."

#define DESC_UPDATE			"Update fastboot3ds. Only signed updates are allowed."
#define DESC_CREDITS    	"Show fastboot3ds credits."

// unused definitions below:
#define LOREM "Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata"

#define DESC_CHANGE_BRIGHT	"Change fastboot3ds brightness. This may also affect firmware launched from within fastboot3ds."
#define DESC_SET_CONTIG		"Enable/disable contiguous NAND backups.\nContiguous NAND backups may be required on certain CFWs to be bootable from SD cards."

#define DESC_ENABLE_CONTIG	"Select to enable contiguous NAND backups."
#define DESC_DISABLE_CONTIG	"Select to disable contiguous NAND backups."


MenuInfo menu_fb3ds[] =
{
	{
		"Main Menu", 6, NULL, 0, 
		{
			{ "Continue boot",				DESC_CONTINUE,				NULL,					MENU_LEAVE_PARAM },
			{ "Boot menu...",				DESC_BOOT_MENU,				NULL,					1 },
			{ "Boot setup...",				DESC_BOOT_SETUP,			NULL,					2 },
			{ "Boot from file...",			DESC_BOOT_FILE,				&menuLaunchFirm,		0xFF },
			{ "NAND tools...",				DESC_NAND_TOOLS,			NULL,					4 },
			{ "Miscellaneous...",			DESC_MISC,	    			NULL,					5 },
			{ "Debug...",					LOREM,	    				NULL,					9 }
		}
	},
	{ // 1
		"Boot Menu", N_BOOTSLOTS, &menuPresetBootMenu, MENU_FLAG_SLOTS,
		{
			SUBENTRY_SLOT_BOOT(1),
			SUBENTRY_SLOT_BOOT(2),
			SUBENTRY_SLOT_BOOT(3),
			SUBENTRY_SLOT_BOOT(4),
			SUBENTRY_SLOT_BOOT(5),
			SUBENTRY_SLOT_BOOT(6)
		}
	},
	{ // 2
		"Boot Setup", N_BOOTSLOTS + 1, &menuPresetBootConfig, MENU_FLAG_SLOTS | MENU_FLAG_BOOTMODE | MENU_FLAG_CONFIG,
		{
			SUBENTRY_SLOT_SETUP(1,6),
			SUBENTRY_SLOT_SETUP(2,6),
			SUBENTRY_SLOT_SETUP(3,6),
			SUBENTRY_SLOT_SETUP(4,6),
			SUBENTRY_SLOT_SETUP(5,6),
			SUBENTRY_SLOT_SETUP(6,6),
			{ "Change boot mode...",		DESC_CHANGE_BOOT,			NULL,					3 }
		}
	},
	{ // 3
		"Boot Mode Setup", 3, &menuPresetBootMode, MENU_FLAG_CONFIG,
		{
			{ "Set normal boot",			DESC_BOOT_NORMAL,			&menuSetBootMode,		0 },
			{ "Set quick boot",				DESC_BOOT_QUICK,			&menuSetBootMode,		1 },
			{ "Set quiet boot",				DESC_BOOT_QUIET,			&menuSetBootMode,		2 }
		}
	},
	{ // 4
		"NAND Tools", 4, &menuPresetNandTools, 0,
		{
			{ "Backup NAND",				DESC_NAND_BACKUP,			&menuBackupNand,		0 },
			{ "Restore NAND",				DESC_NAND_RESTORE,			&menuRestoreNand,		0 },
			{ "Restore NAND (forced)",		DESC_NAND_RESTORE_F,		&menuRestoreNand,		1 },
			{ "Flash firmware to FIRM1",	DESC_FIRM_FLASH,			&menuInstallFirm,		1 }
		}
	},
	{ // 5
		"Miscellaneous", 2, NULL, 0,
		{
			{ "Update fastboot3DS",			DESC_UPDATE,				&menuUpdateFastboot3ds,	0 },
			{ "Credits",					DESC_CREDITS,				&menuShowCredits,		0 }
		}
	},
	SUBMENU_SLOT_SETUP(1), // 6
	SUBMENU_SLOT_SETUP(2), // 7
	SUBMENU_SLOT_SETUP(3), // 8
	SUBMENU_SLOT_SETUP(4), // 9
	SUBMENU_SLOT_SETUP(5), // 10
	SUBMENU_SLOT_SETUP(6), // 11
	/*{ // 12
		"Debug", 2, NULL, 0, // this will not show in the release version
		{
			{ "View current settings",		LOREM,						&debugSettingsView,		0 },
			{ "Escape sequence test",		LOREM,						&debugEscapeTest,		0 } 
		}
	}*/
};
