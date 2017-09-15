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
#include "menu.h"
#include "menu_func.h"

#define LOREM "Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata"

#define DESC_CONTINUE		"Continue booting the first available boot slot.\nNo function if boot slots are not set up."
#define DESC_BOOT_MENU		"Display a boot menu, allowing you to select which boot slot to boot from."
#define DESC_BOOT_FILE		"Select a firmware to boot."
#define DESC_NAND_TOOLS		"Enter NAND tools submenu, including tools for NAND backup, NAND restore and firmware flash."
#define DESC_OPTIONS		"Enter fastboot3ds settings submenu."
#define DESC_MISC			"Enter miscellaneous submenu, including the update tool and credits section."

#define DESC_BOOT_SLOT_1	"Boot slot #1."
#define DESC_BOOT_SLOT_2	"Boot slot #2."
#define DESC_BOOT_SLOT_3	"Boot slot #3."
#define DESC_BOOT_FIRM1		"Boot FIRM1."
#define DESC_CHANGE_SLOT	"Setup fastboot3ds boot slots."

#define DESC_NAND_BACKUP	"Backup current NAND to a file."
#define DESC_NAND_RESTORE	"Restore current NAND from a file.\nThis option preserves your fastboot3ds installation."
#define DESC_NAND_RESTORE_F	"Restore current NAND from a file.\nWARNING: This will overwrite all of your flash memory, also overwriting fastboot3ds installation."
#define DESC_FIRM_FLASH		"Flash firmware from file.\nWARNING: This will allow you to flash unsigned firmware, overwriting fastboot3ds. Using unsupported firmware can BRICK your console."

#define DESC_CHANGE_BOOT	"Change fastboot3ds boot mode. This allows you to set up how your console boots."
#define DESC_CHANGE_BRIGHT	"Change FastBood3DS brightness. This may also affect firmware launched from within fastboot3ds"
#define DESC_SET_CONTIG		"Enable/disable contiguous NAND backups.\nContiguous NAND backups may be required on certain CFWS to be bootable from SD cards."

#define DESC_BOOT_NORMAL	"Set normal boot mode.\nIn normal boot mode, you will be presented with the fastboot3ds menu upon boot."
#define DESC_BOOT_QUICK		"Set quick boot mode.\nIn quick boot mode, splash is displayed and the boot is continued via the first available boot slot. To enter fastboot3ds menu, hold a key combo."
#define DESC_BOOT_QUIET		"Set quiet boot mode.\nIn quiet boot mode, splash is not displayed and the boot is continued via the first available boot slot. To enter fastboot3ds menu, hold a key combo."

#define DESC_SET_SLOT_1		"Setup boot slot #1."
#define DESC_SET_SLOT_2		"Setup boot slot #2."
#define DESC_SET_SLOT_3		"Setup boot slot #3."

#define DESC_ENABLE_CONTIG	"Select to enable contiguous NAND backups."
#define DESC_DISABLE_CONTIG	"Select to disable contiguous NAND backups."

#define DESC_UPDATE			"Update fastboot3ds. Only signed updates are allowed."
#define DESC_CREDITS    	"Show fastboot3ds credits."


MenuInfo menu_fb3ds[] =
{
	{
		"Main Menu", 6,
		{
			{ "Continue boot",				DESC_CONTINUE,				&DummyFunc,				0 },
			{ "Boot menu...",				DESC_BOOT_MENU,				NULL,					1 },
			{ "Boot from file...",			DESC_BOOT_FILE,				&DummyFunc,				2 },
			{ "NAND tools...",				DESC_NAND_TOOLS,			NULL,					3 },
			{ "Settings...",				DESC_OPTIONS,				NULL,					4 },
			{ "Miscellaneous...",			DESC_MISC,	    			NULL,					7 }
		}
	},
	{ // 1
		"Boot Menu", 5,
		{
			{ "Boot [slot 1]",				DESC_BOOT_SLOT_1,			&DummyFunc,				0 },
			{ "Boot [slot 2]",				DESC_BOOT_SLOT_2,			&DummyFunc,				1 },
			{ "Boot [slot 3]",				DESC_BOOT_SLOT_3,			&DummyFunc,				2 },
			{ "Boot from FIRM1",			DESC_BOOT_FIRM1,			&DummyFunc,				2 },
			{ "Setup boot slots...",		DESC_CHANGE_SLOT,			NULL,					2 },
		}
	},
	{ // 2
		"Boot Slot Setup", 3,
		{
			{ "Setup [slot 1]...",			DESC_SET_SLOT_1,			&DummyFunc,				0 },
			{ "Setup [slot 2]...",			DESC_SET_SLOT_2,			&DummyFunc,				1 },
			{ "Setup [slot 3]...",			DESC_SET_SLOT_3,			&DummyFunc,				2 }
		}
	},
	{ // 3
		"NAND Tools", 4,
		{
			{ "Backup NAND",				DESC_NAND_BACKUP,			&DummyFunc,				3 },
			{ "Restore NAND",				DESC_NAND_RESTORE,			&DummyFunc,				4 },
			{ "Restore NAND (forced)",		DESC_NAND_RESTORE_F,		&DummyFunc,				4 },
			{ "Flash firmware",				DESC_FIRM_FLASH,			&DummyFunc,				2 }
		}
	},
	{ // 4
		"Settings", 3,
		{
			{ "Change boot mode",			DESC_CHANGE_BOOT,			NULL,					5 },
			{ "Change brightness",			DESC_CHANGE_BRIGHT,			&DummyFunc,				8 },
			{ "Set contiguous backups",		DESC_SET_CONTIG,			NULL,					6 }
		}
	},
	{ // 5
		"Boot Mode Setup", 3,
		{
			{ "Set normal boot",			DESC_BOOT_NORMAL,			&DummyFunc,				0 },
			{ "Set quick boot",				DESC_BOOT_QUICK,			&DummyFunc,				1 },
			{ "Set quiet boot",				DESC_BOOT_QUIET,			&DummyFunc,				2 }
		}
	},
	{ // 6
		"Backup Setup", 2,
		{
			{ "Enable contiguous backups",	DESC_ENABLE_CONTIG,			&DummyFunc,				1 },
			{ "Disable contiguous backups",	DESC_DISABLE_CONTIG,		&DummyFunc,				0 }
		}
	},
	{ // 7
		"Miscellaneous", 2,
		{
			{ "Update fastboot3DS",			DESC_UPDATE,				&DummyFunc,				1 },
			{ "Credits",					DESC_CREDITS,				&ShowCredits,			0 }
		}
	}
};
