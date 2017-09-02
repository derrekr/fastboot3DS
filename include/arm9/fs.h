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

#include "types.h"
#include "fatfs/ffconf.h"


#define FS_MAX_DRIVES  (FF_VOLUMES)

typedef enum
{
	FS_DRIVE_SDMC = 0,
	FS_DRIVE_TWLN = 1,
	FS_DRIVE_TWLP = 2,
	FS_DRIVE_NAND = 3
} FsDrive;



s32 fMount(FsDrive drive);
s32 fUnmount(FsDrive drive);
