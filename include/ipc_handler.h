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


#define IPC_MAX_PARAMS       (16)
#define IPC_CMD_MASK(cmd)    ((cmd) & 0xFFu)
#define IPC_PARAM_MASK(cmd)  ((cmd)>>16 & 0xFFu)

#define CMD_PARAMS(n)  ((n)<<16)
typedef enum
{
	IPC_CMD9_FMOUNT            = CMD_PARAMS(3) | 0,
	IPC_CMD9_FUNMOUNT          = CMD_PARAMS(1) | 1,
	IPC_CMD9_FOPEN             = CMD_PARAMS(3) | 2,
	IPC_CMD9_FCLOSE            = CMD_PARAMS(1) | 3,
	IPC_CMD9_FREAD             = CMD_PARAMS(4) | 4,
	IPC_CMD9_FWRITE            = CMD_PARAMS(4) | 5,
	IPC_CMD9_FOPEN_DIR         = CMD_PARAMS(2) | 6,
	IPC_CMD9_FREAD_DIR         = CMD_PARAMS(2) | 7,
	IPC_CMD9_FCLOSE_DIR        = CMD_PARAMS(1) | 8,
	IPC_CMD9_FUNLINK           = CMD_PARAMS(1) | 9,
	IPC_CMD9_FGETFREE          = CMD_PARAMS(3) | 10,
	IPC_CMD9_READ_SECTORS      = CMD_PARAMS(17) | 11, // Invalid param num on purpose. Will be decided later.
	IPC_CMD9_WRITE_SECTORS     = CMD_PARAMS(17) | 12, // Invalid param num on purpose. Will be decided later.
	IPC_CMD9_MALLOC            = CMD_PARAMS(1) | 13,
	IPC_CMD9_FREE              = CMD_PARAMS(1) | 14,
	IPC_CMD9_LOAD_VERIFY_FIRM  = CMD_PARAMS(17) | 15, // Invalid param num on purpose. Will be decided later.
	IPC_CMD9_FIRM_LAUNCH       = CMD_PARAMS(17) | 16, // Invalid param num on purpose. Will be decided later.
	IPC_CMD9_PREPA_POWER       = CMD_PARAMS(0) | 17,
	IPC_CMD9_PANIC             = CMD_PARAMS(0) | 18,
	IPC_CMD9_EXCEPTION         = CMD_PARAMS(0) | 19
} IpcCmd9;

typedef enum
{
	IPC_CMD11_PRINT_MSG        = CMD_PARAMS(17) | 0, // Invalid param num on purpose. Will be decided later.
	IPC_CMD11_PANIC            = CMD_PARAMS(17) | 1, // Invalid param num on purpose. Will be decided later.
	IPC_CMD11_EXCEPTION        = CMD_PARAMS(17) | 2  // Invalid param num on purpose. Will be decided later.
} IpcCmd11;
#undef CMD_PARAMS



u32 IPC_handleCmd(u8 cmd, const u32 *buf);
