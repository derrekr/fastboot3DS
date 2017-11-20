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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arm9/dev.h"
#include "arm9/nandimage.h"
#include "util.h"



int validateNandImage(const char *filePath)
{
	FIL file;
	bool isContinuous;
	
	// only nand images on SD card are allowed.
	if(strncmp(filePath, "sdmc:", 5) != 0)
		return NANDIMG_ERROR_BADPATH;
	
	if(f_open(&file, filePath, FA_READ) != FR_OK)
		return NANDIMG_ERROR_NEXISTS;
	
	isContinuous = (file.obj.stat & 3) == 2;
	
	f_close(&file);
	
	if(!isContinuous)
		return NANDIMG_ERROR_NCONTS;
	
	return 0;
}

bool isNandImageCompatible(FIL *file)
{
	NCSD_header imageHeader;
	NCSD_header nandHeader;
	size_t bytesRead;
	
	FSIZE_t offset = f_tell(file);
	
	if(f_lseek(file, 0) != FR_OK)
	{
		return false;
	}
	
	if(f_read(file, &imageHeader, sizeof(NCSD_header), &bytesRead) != FR_OK
		|| bytesRead != sizeof(NCSD_header))
	{
		return false;
	}
	
	// restore original rw pointer
	if(f_lseek(file, offset) != FR_OK)
	{
		return false;
	}
	
	if(!dev_rawnand->read_sector(0, 1, &nandHeader))
	{
		return false;
	}
	
	/* compare everything except the signature */
	return memcmp(&imageHeader.magic, &nandHeader.magic, 0x100) == 0;
}
