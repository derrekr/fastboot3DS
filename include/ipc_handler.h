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


#define IPC_MAX_PARAMS              (15)
#define IPC_CMD_ID_MASK(cmd)        ((cmd)>>24)
#define IPC_CMD_IN_BUFS_MASK(cmd)   ((cmd)>>20 & 0xFu)
#define IPC_CMD_OUT_BUFS_MASK(cmd)  ((cmd)>>16 & 0xFu)
#define IPC_CMD_PARAMS_MASK(cmd)    ((cmd)>>12 & 0xFu)


#define CMD_ID(id)       ((id)<<24)
#define CMD_IN_BUFS(n)   ((n)<<20)
#define CMD_OUT_BUFS(n)  ((n)<<16)
#define CMD_PARAMS(n)    ((n)<<12)

typedef enum
{
	IPC_CMD9_FMOUNT            = CMD_ID(0)  | CMD_IN_BUFS(0)  | CMD_OUT_BUFS(0)  | CMD_PARAMS(1),
	IPC_CMD9_FUNMOUNT          = CMD_ID(1)  | CMD_IN_BUFS(0)  | CMD_OUT_BUFS(0)  | CMD_PARAMS(1),
	IPC_CMD9_FOPEN             = CMD_ID(2)  | CMD_IN_BUFS(1)  | CMD_OUT_BUFS(0)  | CMD_PARAMS(1),
	IPC_CMD9_FCLOSE            = CMD_ID(3)  | CMD_IN_BUFS(0)  | CMD_OUT_BUFS(0)  | CMD_PARAMS(1),
	IPC_CMD9_FREAD             = CMD_ID(4)  | CMD_IN_BUFS(0)  | CMD_OUT_BUFS(1)  | CMD_PARAMS(1),
	IPC_CMD9_FWRITE            = CMD_ID(5)  | CMD_IN_BUFS(1)  | CMD_OUT_BUFS(0)  | CMD_PARAMS(1),
	IPC_CMD9_FOPEN_DIR         = CMD_ID(6)  | CMD_IN_BUFS(0)  | CMD_OUT_BUFS(0)  | CMD_PARAMS(2),
	IPC_CMD9_FREAD_DIR         = CMD_ID(7)  | CMD_IN_BUFS(0)  | CMD_OUT_BUFS(1)  | CMD_PARAMS(2),
	IPC_CMD9_FCLOSE_DIR        = CMD_ID(8)  | CMD_IN_BUFS(0)  | CMD_OUT_BUFS(0)  | CMD_PARAMS(1),
	IPC_CMD9_FUNLINK           = CMD_ID(9)  | CMD_IN_BUFS(1)  | CMD_OUT_BUFS(0)  | CMD_PARAMS(1),
	IPC_CMD9_FGETFREE          = CMD_ID(10) | CMD_IN_BUFS(1)  | CMD_OUT_BUFS(0)  | CMD_PARAMS(3),
	IPC_CMD9_READ_SECTORS      = CMD_ID(11) | CMD_IN_BUFS(15) | CMD_OUT_BUFS(15) | CMD_PARAMS(17), // Invalid on purpose. Will be decided later.
	IPC_CMD9_WRITE_SECTORS     = CMD_ID(12) | CMD_IN_BUFS(15) | CMD_OUT_BUFS(15) | CMD_PARAMS(17), // Invalid on purpose. Will be decided later.
	IPC_CMD9_MALLOC            = CMD_ID(13) | CMD_IN_BUFS(0)  | CMD_OUT_BUFS(0)  | CMD_PARAMS(1),
	IPC_CMD9_FREE              = CMD_ID(14) | CMD_IN_BUFS(0)  | CMD_OUT_BUFS(0)  | CMD_PARAMS(1),
	IPC_CMD9_LOAD_VERIFY_FIRM  = CMD_ID(15) | CMD_IN_BUFS(1)  | CMD_OUT_BUFS(0)  | CMD_PARAMS(17), // Invalid on purpose. Will be decided later.
	IPC_CMD9_FIRM_LAUNCH       = CMD_ID(16) | CMD_IN_BUFS(0)  | CMD_OUT_BUFS(0)  | CMD_PARAMS(17), // Invalid on purpose. Will be decided later.
	IPC_CMD9_PREPA_POWER       = CMD_ID(17) | CMD_IN_BUFS(0)  | CMD_OUT_BUFS(0)  | CMD_PARAMS(0),
	IPC_CMD9_PANIC             = CMD_ID(18) | CMD_IN_BUFS(0)  | CMD_OUT_BUFS(0)  | CMD_PARAMS(0),
	IPC_CMD9_EXCEPTION         = CMD_ID(19) | CMD_IN_BUFS(0)  | CMD_OUT_BUFS(0)  | CMD_PARAMS(0)
} IpcCmd9;

typedef enum
{
	IPC_CMD11_PRINT_MSG        = CMD_ID(0)  | CMD_IN_BUFS(15) | CMD_OUT_BUFS(15) | CMD_PARAMS(17), // Invalid on purpose. Will be decided later.
	IPC_CMD11_PANIC            = CMD_ID(1)  | CMD_IN_BUFS(15) | CMD_OUT_BUFS(15) | CMD_PARAMS(17), // Invalid on purpose. Will be decided later.
	IPC_CMD11_EXCEPTION        = CMD_ID(2)  | CMD_IN_BUFS(15) | CMD_OUT_BUFS(15) | CMD_PARAMS(17)  // Invalid on purpose. Will be decided later.
} IpcCmd11;

#undef CMD_ID
#undef CMD_IN_BUFS
#undef CMD_OUT_BUFS
#undef CMD_PARAMS


typedef struct
{
	void *ptr;
	u32 size;
} IpcBuffer;



u32 IPC_handleCmd(u8 cmdId, u8 inBufs, u8 outBufs, const u32 *const buf);
