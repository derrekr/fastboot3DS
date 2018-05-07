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
#include "ipc_handler.h"
#include "hardware/pxi.h"



s32 writeFirmPartition(const char *const part, bool replaceSig)
{
	u32 cmdBuf[3];
	cmdBuf[0] = (u32)part;
	cmdBuf[1] = strlen(part) + 1;
	cmdBuf[2] = replaceSig;

	return PXI_sendCmd(IPC_CMD9_WRITE_FIRM_PART, cmdBuf, 3);
}

s32 loadVerifyUpdate(const char *const path, u32 *const version)
{
	u32 cmdBuf[4];
	cmdBuf[0] = (u32)path;
	cmdBuf[1] = strlen(path) + 1;
	cmdBuf[2] = (u32)version;
	cmdBuf[3] = sizeof(u32);

	return PXI_sendCmd(IPC_CMD9_LOAD_VERIFY_UPDATE, cmdBuf, 4);
}

s32 toggleSuperhax(bool enable)
{
	const u32 cmdBuf = enable;

	return PXI_sendCmd(IPC_CMD9_TOGGLE_SUPERHAX, &cmdBuf, 1);
}
