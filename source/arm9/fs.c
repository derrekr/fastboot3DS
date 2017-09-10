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
#include "fs.h"
#include "fatfs/ff.h"


static FATFS fsTable[FS_MAX_DRIVES] = {0};
static const char *const fsPathTable[FS_MAX_DRIVES] = {FS_DRIVE_NAMES};
static bool fsStatTable[FS_MAX_DRIVES] = {0};

static FIL fTable[FS_MAX_FILES] = {0};
static bool fStatTable[FS_MAX_FILES] = {0};
static u32 fHandles = 0;

static DIR dTable[FS_MAX_DIRS] = {0};
static bool dStatTable[FS_MAX_DIRS] = {0};
static u32 dHandles = 0;



s32 fMount(FsDrive drive)
{
	if((u32)drive >= FS_MAX_DRIVES) return -30;
	if(fsStatTable[drive]) return -31;

	FRESULT res = f_mount(&fsTable[drive], fsPathTable[drive], 1);
	if(res == FR_OK)
	{
		fsStatTable[drive] = true;
		return FR_OK;
	}
	else return -res;
}

s32 fUnmount(FsDrive drive)
{
	if((u32)drive >= FS_MAX_DRIVES) return -30;
	if(!fsStatTable[drive]) return -31;

	FRESULT res = f_mount(NULL, fsPathTable[drive], 0);
	if(res == FR_OK)
	{
		fsStatTable[drive] = false;
		return FR_OK;
	}
	else return -res;
}

static s32 findUnusedFileSlot(void)
{
	if(fHandles >= FS_MAX_FILES) return -1;

	s32 i = 0;
	while(i < FS_MAX_FILES)
	{
		if(!fStatTable[i]) break;
		i++;
	}

	if(i == FS_MAX_FILES) return -1;
	else return i;
}

static bool isFileHandleValid(s32 handle)
{
	if((u32)handle > fHandles) return false;
	else return true;
}

s32 fOpen(const char *const path, FsOpenMode mode)
{
	const s32 i = findUnusedFileSlot();
	if(i < 0) return -30;

	FRESULT res = f_open(&fTable[i], path, mode);
	if(res == FR_OK)
	{
		fStatTable[i] = true;
		fHandles++;
		return i; // Handle
	}
	else return -res;
}

s32 fRead(s32 handle, void *const buf, u32 size)
{
	if(!isFileHandleValid(handle)) return -30;

	UINT bytesRead;
	FRESULT res = f_read(&fTable[handle], buf, size, &bytesRead);

	if(bytesRead != size) return -31;
	if(res == FR_OK) return FR_OK;
	else return -res;
}

s32 fWrite(s32 handle, const void *const buf, u32 size)
{
	if(!isFileHandleValid(handle)) return -30;

	UINT bytesWritten;
	FRESULT res = f_write(&fTable[handle], buf, size, &bytesWritten);

	if(bytesWritten != size) return -31;
	if(res == FR_OK) return FR_OK;
	else return -res;
}

s32 fSync(s32 handle)
{
	if(!isFileHandleValid(handle)) return -30;

	FRESULT res = f_sync(&fTable[handle]);
	if(res == FR_OK) return res;
	else return -res;
}

s32 fLseek(s32 handle, u32 offset)
{
	if(!isFileHandleValid(handle)) return -30;

	FRESULT res = f_lseek(&fTable[handle], offset);
	if(res == FR_OK) return res;
	else return -res;
}

u32 fTell(s32 handle)
{
	if(!isFileHandleValid(handle)) return 0;
	return f_tell(&fTable[handle]);
}

u32 fSize(s32 handle)
{
	if(!isFileHandleValid(handle)) return 0;
	return f_size(&fTable[handle]);
}

s32 fClose(s32 handle)
{
	if(fHandles == 0 || !isFileHandleValid(handle)) return -30;

	FRESULT res = f_close(&fTable[handle]);
	if(res == FR_OK)
	{
		fStatTable[handle] = false;
		fHandles--;
		return FR_OK;
	}
	else return -res;
}

s32 fStat(const char *const path, FsFileInfo *fi)
{
	FRESULT res = f_stat(path, fi);
	if(res == FR_OK) return res;
	else return -res;
}

s32 fExpand(s32 handle, u32 size)
{
	if(!isFileHandleValid(handle)) return -30;

	FRESULT res = f_expand(&fTable[handle], size, 1);
	if(res == FR_OK) return res;
	else return -res;
}

static s32 findUnusedDirSlot(void)
{
	if(dHandles >= FS_MAX_DIRS) return -1;

	s32 i = 0;
	while(i < FS_MAX_DIRS)
	{
		if(!dStatTable[i]) break;
		i++;
	}

	if(i == FS_MAX_DIRS) return -1;
	else return i;
}

static bool isDirHandleValid(s32 handle)
{
	if((u32)handle > dHandles) return false;
	else return true;
}

s32 fOpenDir(const char *const path)
{
	const s32 i = findUnusedDirSlot();
	if(i < 0) return -30;

	FRESULT res = f_opendir(&dTable[i], path);
	if(res == FR_OK)
	{
		dStatTable[i] = true;
		dHandles++;
		return i; // Handle
	}
	else return -res;
}

s32 fReadDir(s32 handle, FsFileInfo *fi, u32 num)
{
	if(!isDirHandleValid(handle)) return -30;
	if(num > 1000) return -31;

	u32 i;
	for(i = 0; i < num; i++)
	{
		FRESULT res = f_readdir(&dTable[handle], &fi[i]);
		if(res != FR_OK) return -res;
		if(!fi[i].fname[0]) break;
	}

	return i;
}

s32 fCloseDir(s32 handle)
{
	if(dHandles == 0 || !isDirHandleValid(handle)) return -30;

	FRESULT res = f_closedir(&dTable[handle]);
	if(res == FR_OK)
	{
		dStatTable[handle] = false;
		dHandles--;
		return FR_OK;
	}
	else return -res;
}

s32 fMkdir(const char *const path)
{
	FRESULT res = f_mkdir(path);
	if(res == FR_OK) return res;
	else return -res;
}

s32 fRename(const char *const old, const char *const new)
{
	FRESULT res = f_rename(old, new);
	if(res == FR_OK) return res;
	else return -res;
}

s32 fUnlink(const char *const path)
{
	FRESULT res = f_unlink(path);
	if(res == FR_OK) return res;
	else return -res;
}
