#pragma once

/*
 *   This file is part of fastboot 3DS
 *   Copyright (C) 2017 derrek, profi200
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

#include "arm9/debug.h"
#include "fatfs/ff.h"
#include "arm9/ui.h"


enum bootOptionResultTypes {
	BO_NOT_ATTEMPTED = 0,
	BO_NOT_FOUND,
	BO_FAILED,
	BO_SKIPPED
};

typedef struct {
	char  model[0x10];
	bool  unitIsNew3DS;
	char  bootEnv[0x20];
	char  mode[7];
	char  fw_ver1[10];
	char  fw_ver2[10];
	u32   numBootOptionsAttempted;
	u32   bootOptionResults[3];
	bool  wififlash_status;
	u32   nand_status;      // Bit 0: twln, bit 1: twlp, bit 2: nand
	u32   sd_status;        // 0: Not inserted, 1: FS init failed, 2: OK
} bootInfoStruct;

bootInfoStruct bootInfo;



void devs_close();
u8 rng_get_byte();
noreturn void power_off_safe();
noreturn void reboot_safe();
