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
// #include "arm11/dev.h"
#include "arm11/fsutils.h"
#include "util.h"
#include "fs.h"

bool fsEnsureMounted(const char *path)
{
	if(strncmp(path, "sdmc:", 5) == 0)
	{
		return (fMount(FS_DRIVE_SDMC) == 0);
	}
	else if(strncmp(path, "nand:", 5) == 0)
	{
		return (fMount(FS_DRIVE_NAND) == 0);
	}
	else if(strncmp(path, "twln:", 5) == 0)
	{
		return (fMount(FS_DRIVE_TWLN) == 0);
	}
	else if(strncmp(path, "twlp:", 5) == 0)
	{
		return (fMount(FS_DRIVE_TWLP) == 0);
	}
	
	return false;
}

void fsUnmountAll()
{
	fUnmount(FS_DRIVE_SDMC);
	fUnmount(FS_DRIVE_NAND);
	fUnmount(FS_DRIVE_TWLN);
	fUnmount(FS_DRIVE_TWLP);
}

u32 fsMountNandFilesystems()
{
	u32 res = 0;

	res |= (fMount(FS_DRIVE_TWLN) ? 0 : 1u);
	res |= (fMount(FS_DRIVE_TWLP) ? 0 : 1u<<1);
	res |= (fMount(FS_DRIVE_NAND) ? 0 : 1u<<2);

	return res;
}

void fsUnmountNandFilesystems()
{
	fUnmount(FS_DRIVE_NAND);
	fUnmount(FS_DRIVE_TWLN);
	fUnmount(FS_DRIVE_TWLP);
	// dev_decnand->close(); // ?
}

bool fsMountSdmc()
{
	return (fMount(FS_DRIVE_SDMC) == 0);
}

bool fsCreateFileWithPath(const char *filepath)
{
	const size_t maxBufSize = 0x100;
	char *tempBuf = (char *) malloc(maxBufSize);
	char *tempPtr = tempBuf;
	s32 fhandle;
	FILINFO fno;
	
	/* memory check */
	if (!tempBuf)
	{
		return false; // out of memory
	}
	
	/* create directories */
	for (char *p = (char*) filepath; *p; p++)
	{
		if ((*p == '/') || (*p == '\\'))
		{
			*tempPtr = '\0';
			s32 dhandle = fOpenDir(tempBuf);
			if (dhandle >= 0) fCloseDir(dhandle);
			else if (fMkdir(tempBuf) != 0) goto fail;
		}
		*(tempPtr++) = *p;
	}
	*tempPtr = '\0';
	
	/* touch file */
	if (fStat(tempBuf, &fno) == 0)
	{
		if (fno.fattrib & AM_DIR) goto fail;
	}
	
	fhandle = fOpen(tempBuf, FS_CREATE_ALWAYS);
	if (fhandle < 0) goto fail;
	fClose(fhandle);
	
	free(tempBuf);
	
	return true;
	
fail:
	
	free(tempBuf);
	
	return false;
}

