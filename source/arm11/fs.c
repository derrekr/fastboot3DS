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

#include <string.h>
#include "types.h"
#include "fs.h"
#include "ipc_handler.h"
#include "hardware/pxi.h"



s32 fMount(FsDrive drive)
{
	const u32 cmdBuf = drive;
	return PXI_sendCmd(IPC_CMD9_FMOUNT, &cmdBuf, 1);
}

s32 fUnmount(FsDrive drive)
{
	const u32 cmdBuf = drive;
	return PXI_sendCmd(IPC_CMD9_FUNMOUNT, &cmdBuf, 1);
}

bool fIsDriveMounted(FsDrive drive)
{
	const u32 cmdBuf = drive;
	return PXI_sendCmd(IPC_CMD9_FIS_DRIVE_MOUNTED, &cmdBuf, 1);
}

s32 fGetFree(FsDrive drive, u64 *size)
{
	u32 cmdBuf[3];
	cmdBuf[0] = (u32)size;
	cmdBuf[1] = 8;
	cmdBuf[2] = drive;

	return PXI_sendCmd(IPC_CMD9_FGETFREE, cmdBuf, 3);
}

u32 fGetDeviceSize(FsDevice dev)
{
	const u32 cmdBuf = dev;
	return PXI_sendCmd(IPC_CMD9_FGET_DEV_SIZE, &cmdBuf, 1);
}

bool fIsDevActive(FsDevice dev)
{
	const u32 cmdBuf = dev;
	return PXI_sendCmd(IPC_CMD9_FIS_DEV_ACTIVE, &cmdBuf, 1);
}

s32 fPrepareRawAccess(FsDevice dev)
{
	const u32 cmdBuf = dev;
	return PXI_sendCmd(IPC_CMD9_FPREP_RAW_ACCESS, &cmdBuf, 1);
}

s32 fFinalizeRawAccess(DevHandle handle)
{
	const u32 cmdBuf = handle;
	return PXI_sendCmd(IPC_CMD9_FFINAL_RAW_ACCESS, &cmdBuf, 1);
}

s32 fCreateDeviceBuffer(u32 size)
{
	const u32 cmdBuf = size;
	return PXI_sendCmd(IPC_CMD9_FCREATE_DEV_BUF, &cmdBuf, 1);
}

s32 fFreeDeviceBuffer(DevBufHandle handle)
{
	const u32 cmdBuf = handle;
	return PXI_sendCmd(IPC_CMD9_FFREE_DEV_BUF, &cmdBuf, 1);
}

s32 fReadToDeviceBuffer(s32 sourceHandle, u32 sourceOffset, u32 sourceSize, DevBufHandle devBufHandle)
{
	u32 cmdBuf[4];
	cmdBuf[0] = sourceHandle;
	cmdBuf[1] = sourceOffset;
	cmdBuf[2] = sourceSize;
	cmdBuf[3] = devBufHandle;

	return PXI_sendCmd(IPC_CMD9_FREAD_TO_DEV_BUF, cmdBuf, 4);
}

s32 fsWriteFromDeviceBuffer(s32 destHandle, u32 destOffset, u32 destSize, DevBufHandle devBufHandle)
{
	u32 cmdBuf[4];
	cmdBuf[0] = destHandle;
	cmdBuf[1] = destOffset;
	cmdBuf[2] = destSize;
	cmdBuf[3] = devBufHandle;

	return PXI_sendCmd(IPC_CMD9_FWRITE_FROM_DEV_BUF, cmdBuf, 4);
}

s32 fOpen(const char *const path, FsOpenMode mode)
{
	u32 cmdBuf[3];
	cmdBuf[0] = (u32)path;
	cmdBuf[1] = strlen(path) + 1;
	cmdBuf[2] = mode;

	return PXI_sendCmd(IPC_CMD9_FOPEN, cmdBuf, 3);
}

s32 fRead(s32 handle, void *const buf, u32 size)
{
	u32 cmdBuf[3];
	cmdBuf[0] = (u32)buf;
	cmdBuf[1] = size;
	cmdBuf[2] = handle;

	return PXI_sendCmd(IPC_CMD9_FREAD, cmdBuf, 3);
}

s32 fWrite(s32 handle, const void *const buf, u32 size)
{
	u32 cmdBuf[3];
	cmdBuf[0] = (u32)buf;
	cmdBuf[1] = size;
	cmdBuf[2] = handle;

	return PXI_sendCmd(IPC_CMD9_FWRITE, cmdBuf, 3);
}

s32 fSync(s32 handle)
{
	const u32 cmdBuf = handle;
	return PXI_sendCmd(IPC_CMD9_FSYNC, &cmdBuf, 1);
}

s32 fLseek(s32 handle, u32 offset)
{
	u32 cmdBuf[2];
	cmdBuf[0] = handle;
	cmdBuf[1] = offset;

	return PXI_sendCmd(IPC_CMD9_FLSEEK, cmdBuf, 2);
}

u32 fTell(s32 handle)
{
	const u32 cmdBuf = handle;
	return PXI_sendCmd(IPC_CMD9_FTELL, &cmdBuf, 1);
}

u32 fSize(s32 handle)
{
	const u32 cmdBuf = handle;
	return PXI_sendCmd(IPC_CMD9_FSIZE, &cmdBuf, 1);
}

s32 fClose(s32 handle)
{
	const u32 cmdBuf = handle;
	return PXI_sendCmd(IPC_CMD9_FCLOSE, &cmdBuf, 1);
}

s32 fExpand(s32 handle, u32 size)
{
	u32 cmdBuf[2];
	cmdBuf[0] = handle;
	cmdBuf[1] = size;

	return PXI_sendCmd(IPC_CMD9_FEXPAND, cmdBuf, 2);
}

s32 fStat(const char *const path, FsFileInfo *fi)
{
	u32 cmdBuf[4];
	cmdBuf[0] = (u32)path;
	cmdBuf[1] = strlen(path) + 1;
	cmdBuf[2] = (u32)fi;
	cmdBuf[3] = sizeof(FsFileInfo);

	return PXI_sendCmd(IPC_CMD9_FSTAT, cmdBuf, 4);
}

s32 fOpenDir(const char *const path)
{
	u32 cmdBuf[2];
	cmdBuf[0] = (u32)path;
	cmdBuf[1] = strlen(path) + 1;

	return PXI_sendCmd(IPC_CMD9_FOPEN_DIR, cmdBuf, 2);
}

s32 fReadDir(s32 handle, FsFileInfo *fi, u32 num)
{
	u32 cmdBuf[4];
	cmdBuf[0] = (u32)fi;
	cmdBuf[1] = sizeof(FsFileInfo) * num;
	cmdBuf[2] = handle;
	cmdBuf[3] = num;

	return PXI_sendCmd(IPC_CMD9_FREAD_DIR, cmdBuf, 4);
}

s32 fCloseDir(s32 handle)
{
	const u32 cmdBuf = handle;
	return PXI_sendCmd(IPC_CMD9_FCLOSE_DIR, &cmdBuf, 1);
}

s32 fMkdir(const char *const path)
{
	u32 cmdBuf[2];
	cmdBuf[0] = (u32)path;
	cmdBuf[1] = strlen(path) + 1;

	return PXI_sendCmd(IPC_CMD9_FMKDIR, cmdBuf, 2);
}

s32 fRename(const char *const old, const char *const new)
{
	u32 cmdBuf[4];
	cmdBuf[0] = (u32)old;
	cmdBuf[1] = strlen(old) + 1;
	cmdBuf[2] = (u32)new;
	cmdBuf[3] = strlen(new) + 1;

	return PXI_sendCmd(IPC_CMD9_FRENAME, cmdBuf, 4);
}

s32 fUnlink(const char *const path)
{
	u32 cmdBuf[2];
	cmdBuf[0] = (u32)path;
	cmdBuf[1] = strlen(path) + 1;

	return PXI_sendCmd(IPC_CMD9_FUNLINK, cmdBuf, 2);
}

s32 fVerifyNandImage(const char *const path)
{
	u32 cmdBuf[2];
	cmdBuf[0] = (u32)path;
	cmdBuf[1] = strlen(path) + 1;

	return PXI_sendCmd(IPC_CMD9_FVERIFY_NAND_IMG, cmdBuf, 2);
}

s32 fSetNandProtection(bool protect)
{
	const u32 cmdBuf = protect;
	return PXI_sendCmd(IPC_CMD9_FSET_NAND_PROT, &cmdBuf, 1);
}
