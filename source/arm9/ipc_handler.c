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

#include <stdlib.h>
#include "types.h"
#include "ipc_handler.h"
#include "hardware/cache.h"
#include "arm9/debug.h"
#include "fs.h"
#include "arm9/firm.h"



u32 IPC_handleCmd(u8 cmdId, u32 inBufs, u32 outBufs, const u32 *const buf)
{
	for(u32 i = 0; i < inBufs; i++)
	{
		const IpcBuffer *const inBuf = (IpcBuffer*)&buf[i * sizeof(IpcBuffer) / 4];
		invalidateDCacheRange(inBuf->ptr, inBuf->size);
	}

	u32 result = 0;
	switch(cmdId)
	{
		case IPC_CMD_ID_MASK(IPC_CMD9_FMOUNT):
			result = fMount(buf[0]);
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FUNMOUNT):
			result = fUnmount(buf[0]);
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FGETFREE):
			result = fGetFree(buf[2], (u64*)buf[0]);
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FOPEN):
			result = fOpen((const char *const)buf[0], buf[2]);
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FREAD):
			result = fRead(buf[2], (void *const)buf[0], buf[1]);
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FWRITE):
			result = fWrite(buf[2], (const void *const)buf[0], buf[1]);
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FSYNC):
			result = fSync(buf[0]);
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FLSEEK):
			result = fLseek(buf[0], buf[1]);
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FTELL):
			result = fTell(buf[0]);
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FSIZE):
			result = fSize(buf[0]);
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FCLOSE):
			result = fClose(buf[0]);
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FEXPAND):
			result = fExpand(buf[0], buf[1]);
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FSTAT):
			result = fStat((const char *const)buf[0], (FsFileInfo*)buf[2]);
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FOPEN_DIR):
			result = fOpenDir((const char *const)buf[0]);
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FREAD_DIR):
			result = fReadDir(buf[2], (FsFileInfo*)buf[0], buf[3]);
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FCLOSE_DIR):
			result = fCloseDir(buf[0]);
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FMKDIR):
			result = fMkdir((const char *const)buf[0]);
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FRENAME):
			result = fRename((const char *const)buf[0], (const char *const)buf[2]);
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FUNLINK):
			result = fUnlink((const char *const)buf[0]);
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_READ_SECTORS):
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_WRITE_SECTORS):
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_MALLOC):
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FREE):
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_LOAD_VERIFY_FIRM):
			result = loadVerifyFirm((const char *const)buf[0], buf[2]);
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FIRM_LAUNCH):
			{
				extern volatile bool g_startFirmLaunch;
				g_startFirmLaunch = true;
			}
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_PREPA_POWER):
		case IPC_CMD_ID_MASK(IPC_CMD9_PANIC):
		case IPC_CMD_ID_MASK(IPC_CMD9_EXCEPTION):
			// Close and deinitalize everything.
			break;
		default:
			panic();
	}

	for(u32 i = inBufs; i < inBufs + outBufs; i++)
	{
		const IpcBuffer *const outBuf = (IpcBuffer*)&buf[i * sizeof(IpcBuffer) / 4];
		flushDCacheRange(outBuf->ptr, outBuf->size);
	}

	return result;
}
