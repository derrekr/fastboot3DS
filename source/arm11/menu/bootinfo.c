/*
 *   This file is part of fastboot 3DS
 *   Copyright (C) 2017 myria, derrek, profi200, d0k3
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

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "arm11/hardware/mcu.h"
#include "arm11/menu/bootinfo.h"
#include "arm11/bootenv.h"
#include "arm11/fmt.h"
#include "fs.h"


// System models.
enum SystemModel {
	MODEL_OLD_3DS = 0,
	MODEL_OLD_3DS_XL,
	MODEL_NEW_3DS,
	MODEL_OLD_2DS,
	MODEL_NEW_3DS_XL,
	MODEL_NEW_2DS_XL,
	NUM_MODELS
};

// Table of system models.
// https://www.3dbrew.org/wiki/Cfg:GetSystemModel#System_Model_Values
static const struct {
	char name[12];
	char product_code[4];
} s_modelNames[] = {
	{ "Old 3DS",	"CTR" }, // 0
	{ "Old 3DS XL", "SPR" }, // 1
	{ "New 3DS",	"KTR" }, // 2
	{ "Old 2DS",	"FTR" }, // 3
	{ "New 3DS XL", "RED" }, // 4
	{ "New 2DS XL", "JAN" }, // 5
};

void getBootInfo(bootInfo* info)
{
	// System model.
	static u8 int_model = 0xFF;
	ee_snprintf(info->model, 24, "<unknown model>");

	// Get MCU system information.
	if (int_model >= NUM_MODELS)
		int_model = MCU_getSystemModel();
	if (int_model < NUM_MODELS)
		ee_snprintf(info->model, 24, "%s", s_modelNames[int_model].name);
	
	// Boot environment.
	u32 bootEnv = getBootEnv();
	strncpy(info->bootEnv,
		(bootEnv == BOOTENV_COLD_BOOT) ? "Cold boot" :
		(bootEnv == BOOTENV_NATIVE_FIRM) ? "NATIVE_FIRM reboot" :
		(bootEnv == BOOTENV_TWL_FIRM) ? "TWL_FIRM reboot" :
		(bootEnv == BOOTENV_AGB_FIRM) ? "AGB_FIRM reboot" : "<unknown boot>",
		24);
		
	// Mounted drives.
	info->mountState = 0;
	for (FsDrive i = FS_DRIVE_SDMC; i <= FS_DRIVE_NAND; i++)
		info->mountState |= (fIsDriveMounted(i) ? 1 : 0) << i;
}
