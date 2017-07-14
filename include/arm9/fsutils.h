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
#include "fatfs/ff.h"


// SD card FAT fs instance
FATFS sd_fs;
// same for all NAND filesystems
FATFS nand_twlnfs, nand_twlpfs, nand_fs;



bool fsGetFreeSpaceOnDrive(const char *drive, u64 *freeSpace);
bool fsEnsureMounted(const char *path);
void fsUnmountAll();
u32 fsMountNandFilesystems();
void fsUnmountNandFilesystems();
u32 fsRemountNandFilesystems();
bool fsMountSdmc();
bool fsCreateFileWithPath(const char *filepath);
