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
#include "mem_map.h"
#include "ipc_handler.h"
#include "hardware/pxi.h"
#include "arm9/debug.h"
#include "fatfs/ff.h"



u32 IPC_handleCmd(u8 cmd, const u32 *buf)
{
	u32 result = 0;

	switch(cmd)
	{
		case IPC_CMD_MASK(IPC_CMD9_FMOUNT):
			break;
		case IPC_CMD_MASK(IPC_CMD9_FUNMOUNT):
			break;
		case IPC_CMD_MASK(IPC_CMD9_FOPEN):
			break;
		case IPC_CMD_MASK(IPC_CMD9_FCLOSE):
			break;
		case IPC_CMD_MASK(IPC_CMD9_FREAD):
			break;
		case IPC_CMD_MASK(IPC_CMD9_FWRITE):
			break;
		case IPC_CMD_MASK(IPC_CMD9_FOPEN_DIR):
			break;
		case IPC_CMD_MASK(IPC_CMD9_FREAD_DIR):
			break;
		case IPC_CMD_MASK(IPC_CMD9_FCLOSE_DIR):
			break;
		case IPC_CMD_MASK(IPC_CMD9_FUNLINK):
			break;
		case IPC_CMD_MASK(IPC_CMD9_FGETFREE):
			break;
		case IPC_CMD_MASK(IPC_CMD9_READ_SECTORS):
			break;
		case IPC_CMD_MASK(IPC_CMD9_WRITE_SECTORS):
			break;
		case IPC_CMD_MASK(IPC_CMD9_MALLOC):
			result = (u32)malloc(buf[0]);
			break;
		case IPC_CMD_MASK(IPC_CMD9_FREE):
			{
				extern u32 __end__[];
				if(buf[0] > A9_HEAP_END || buf[0] < (u32)__end__) panic();
				free((void*)buf[0]);
			}
			break;
		case IPC_CMD_MASK(IPC_CMD9_LOAD_VERIFY_FIRM):
			break;
		case IPC_CMD_MASK(IPC_CMD9_FIRM_LAUNCH):
			break;
		case IPC_CMD_MASK(IPC_CMD9_PREPA_POWER):
		case IPC_CMD_MASK(IPC_CMD9_PANIC):
		case IPC_CMD_MASK(IPC_CMD9_EXCEPTION):
			// Close and deinitalize everything.
			break;
		default:
			panic();
	}

	return result;
}
