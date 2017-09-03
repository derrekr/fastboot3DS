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

s32 fOpen(const char *const path, FsOpenMode mode)
{
	u32 cmdBuf[3];
	cmdBuf[0] = (u32)path;
	cmdBuf[1] = strlen(path);
	cmdBuf[2] = mode;

	return PXI_sendCmd(IPC_CMD9_FOPEN, cmdBuf, 3);
}

s32 fClose(s32 handle)
{
	const u32 cmdBuf = handle;
	return PXI_sendCmd(IPC_CMD9_FCLOSE, &cmdBuf, 1);
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
