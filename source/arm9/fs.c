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

#include "types.h"
#include "arm9/fs.h"
#include "fatfs/ff.h"


static FATFS fsTable[FS_MAX_DRIVES] = {0};
static const char *const fsPathTable[FS_MAX_DRIVES] = {"sdmc:/", "twln:/", "twlp:/", "nand:/"};
static bool fsStatTable[FS_MAX_DRIVES] = {0};



s32 fMount(FsDrive drive)
{
	if(drive >= FS_MAX_DRIVES) return -30;

	if(fsStatTable[drive]) return -30;

	FRESULT res = f_mount(&fsTable[drive], fsPathTable[drive], 1);
	if(res == FR_OK)
	{
		fsStatTable[drive] = true;
		return drive; // Handle
	}
	else return -res;
}

s32 fUnmount(FsDrive drive)
{
	if(drive >= FS_MAX_DRIVES) return -30;

	if(!fsStatTable[drive]) return -30;

	FRESULT res = f_mount(NULL, fsPathTable[drive], 1);
	if(res == FR_OK)
	{
		fsStatTable[drive] = false;
		return FR_OK;
	}
	else return -res;
}
