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

#define DESC_CONTINUE		"Continue booting the first available boot slot.\nNo function if boot slots are not set up or firmware not found."
#define DESC_BOOT_MENU		"Display a boot menu, allowing you to select which boot slot to boot from."
#define DESC_LAUNCH			"Select a firmware to launch."
#define DESC_NAND_TOOLS		"Enter NAND tools submenu, including tools for NAND backup, NAND restore and firmware flash."
#define DESC_OPTIONS		"Enter FastBoot3DS options submenu."
#define DESC_UPDATE			"Update FastBoot3DS. Only signed updates are allowed."
#define DESC_CREDITS    	"Show FastBoot3DS credits."

#define DESC_NAND_BACKUP	"Backup current NAND to a file."
#define DESC_NAND_RESTORE	"Restore current NAND from a file.\nThis option preserves your FastBoot3DS installation."
#define DESC_NAND_RESTORE_F	"Restore current NAND from a file.\nWARNING: This will overwrite all of your flash memory, also overwriting FastBoot3DS installation."
#define DESC_FIRM_FLASH		"Flash firmware from file.\nWARNING: This will allow you to flash unsigned firmware, overwriting FastBoot3DS. Using unsupported firmware can BRICK your console."

#define DESC_CHANGE_BOOT	"Change FastBoot3DS boot mode. This allows you to set up how your console boots."
#define DESC_CHANGE_SLOT	"Setup FastBoot3DS boot slots."
#define DESC_CHANGE_BRIGHT	"Change FastBood3DS brightness. This may also affect firmware launched from within FastBoot3DS"
#define DESC_SET_CONTIG		"Enable/disable contiguous NAND backups. Contiguous NAND backups may be required on certain CFWS to be bootable from SD cards"

#define DESC_BOOT_NORMAL	"Set normal boot mode.\nIn normal boot mode, you will be presented with the FastBoot3DS menu upon boot."
#define DESC_BOOT_QUICK		"Set quick boot mode.\nIn quick boot mode, splash is displayed and the boot is continued via the first available boot slot. To enter FastBoot3DS menu, hold a key combo."
#define DESC_BOOT_QUIET		"Set quiet boot mode.\nIn quiet boot mode, splash is not displayed and the boot is continued via the first available boot slot. To enter FastBoot3DS menu, hold a key combo."

#define DESC_SLOT_1			"Setup boot slot #1"
#define DESC_SLOT_2			"Setup boot slot #2"
#define DESC_SLOT_3			"Setup boot slot #3"

#define DESC_ENABLE_CONTIG	"Enable contiguous NAND backups. Contiguous NAND backups may be required on certain CFWS to be bootable from SD cards"
#define DESC_DISABLE_CONTIG	"Disable contiguous NAND backups. Contiguous NAND backups may be required on certain CFWS to be bootable from SD cards"


MenuInfo menu_fb3ds[] =
{
	{
		"FastBoot3DS Main Menu", 7,
		{
			{ "Continue boot",				DESC_CONTINUE,				&DummyFunc,				0 },
			{ "Boot menu..",				DESC_BOOT_MENU,				&DummyFunc,				0 },
			{ "Launch firmware...",			DESC_LAUNCH,				&DummyFunc,				1 },
			{ "NAND tools...",				DESC_NAND_TOOLS,			NULL,					1 },
			{ "Options...",					DESC_OPTIONS,				NULL,					2 },
			{ "Update...",					DESC_UPDATE,				&DummyFunc,				1 },
			{ "Credits",					DESC_CREDITS,    			&DummyFunc,				3 }
		}
	},
	{
		"FastBoot3DS NAND tools", 4,
		{
			{ "Backup NAND",				DESC_NAND_BACKUP,			&DummyFunc,				3 },
			{ "Restore NAND",				DESC_NAND_RESTORE,			&DummyFunc,				4 },
			{ "Restore NAND (forced)",		DESC_NAND_RESTORE_F,		&DummyFunc,				4 },
			{ "Flash firmware",				DESC_FIRM_FLASH,			&DummyFunc,				2 }
		}
	},
	{
		"FastBoot3DS options", 4,
		{
			{ "Change boot mode",			DESC_CHANGE_BOOT,			NULL,					3 },
			{ "Setup boot slot",			DESC_CHANGE_SLOT,			NULL,					4 },
			{ "Change brightness",			DESC_CHANGE_BRIGHT,			&DummyFunc,				6 },
			{ "Force contiguous backups",	DESC_SET_CONTIG,			NULL,					5 }
		}
	},
	{
		"FastBoot3DS boot mode", 3,
		{
			{ "Set normal boot",			DESC_BOOT_NORMAL,			&DummyFunc,				0 },
			{ "Set quick boot",				DESC_BOOT_QUICK,			&DummyFunc,				1 },
			{ "Set quiet boot",				DESC_BOOT_QUIET,			&DummyFunc,				2 }
		}
	},
	{
		"FastBoot3DS boot slots", 3,
		{
			{ "Setup [slot 1]...",			DESC_SLOT_1,				&DummyFunc,				0 },
			{ "Setup [slot 2]...",			DESC_SLOT_2,				&DummyFunc,				1 },
			{ "Setup [slot 3]...",			DESC_SLOT_3,				&DummyFunc,				2 }
		}
	},
	{
		"FastBoot3DS backup setting", 2,
		{
			{ "Enable contiguous backups",	DESC_ENABLE_CONTIG,			&DummyFunc,				1 },
			{ "Disable contiguous backups",	DESC_DISABLE_CONTIG,		&DummyFunc,				0 }
		}
	}
};
