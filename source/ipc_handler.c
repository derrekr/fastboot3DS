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
#ifdef ARM9
	#include "arm9/debug.h"
	#include "fatfs/ff.h"
#endif


#ifdef ARM11
// Temporary until we have a panic() function.
#define panic()  *((vu32*)4) = 0xDEADBEEF
#endif



#ifdef ARM9
u32 IPC_handleCmd(u8 cmdId, u8 inBufs, u8 outBufs, const u32 *const buf)
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
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FUNMOUNT):
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FOPEN):
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FCLOSE):
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FREAD):
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FWRITE):
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FOPEN_DIR):
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FREAD_DIR):
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FCLOSE_DIR):
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FUNLINK):
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FGETFREE):
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_READ_SECTORS):
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_WRITE_SECTORS):
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_MALLOC):
			//result = (u32)malloc(buf[0]);
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FREE):
			/*{
				extern char* fake_heap_start;
				extern char* fake_heap_end;
				if(buf[0] > (u32)fake_heap_end || buf[0] < (u32)fake_heap_start) panic();
				free((void*)buf[0]);
			}*/
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_LOAD_VERIFY_FIRM):
			break;
		case IPC_CMD_ID_MASK(IPC_CMD9_FIRM_LAUNCH):
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
		const IpcBuffer *const outBuf = (IpcBuffer*)&buf[i * sizeof(IpcBuffer)];
		flushDCacheRange(outBuf->ptr, outBuf->size);
	}

	return result;
}

#elif ARM11

u32 IPC_handleCmd(u8 cmdId, u8 inBufs, u8 outBufs, UNUSED const u32 *const buf)
{
	for(u32 i = 0; i < inBufs; i++)
	{
		const IpcBuffer *const inBuf = (IpcBuffer*)&buf[i * sizeof(IpcBuffer) / 4];
		invalidateDCacheRange(inBuf->ptr, inBuf->size);
	}

	u32 result = 0;
	switch(cmdId)
	{
		case IPC_CMD_ID_MASK(IPC_CMD11_PRINT_MSG):
			break;
		case IPC_CMD_ID_MASK(IPC_CMD11_PANIC):
			break;
		case IPC_CMD_ID_MASK(IPC_CMD11_EXCEPTION):
			break;
		default:
			panic();
	}

	for(u32 i = inBufs; i < inBufs + outBufs; i++)
	{
		const IpcBuffer *const outBuf = (IpcBuffer*)&buf[i * sizeof(IpcBuffer)];
		flushDCacheRange(outBuf->ptr, outBuf->size);
	}

	return result;
}
#endif
