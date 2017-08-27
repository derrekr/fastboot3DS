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


typedef enum
{
	IPC_CMD9_FMOUNT            = 0,
	IPC_CMD9_FUNMOUNT          = 1,
	IPC_CMD9_FOPEN             = 2,
	IPC_CMD9_FCLOSE            = 3,
	IPC_CMD9_FREAD             = 4,
	IPC_CMD9_FWRITE            = 5,
	IPC_CMD9_FOPEN_DIR         = 6,
	IPC_CMD9_FREAD_DIR         = 7,
	IPC_CMD9_FCLOSE_DIR        = 8,
	IPC_CMD9_FUNLINK           = 9,
	IPC_CMD9_FGETFREE          = 10,
	IPC_CMD9_READ_SECTORS      = 11,
	IPC_CMD9_WRITE_SECTORS     = 12,
	IPC_CMD9_MALLOC            = 13,
	IPC_CMD9_FREE              = 14,
	IPC_CMD9_LOAD_VERIFY_FIRM  = 15,
	IPC_CMD9_FIRM_LAUNCH       = 16,
	IPC_CMD9_PREPA_POWER       = 17,
	IPC_CMD9_PANIC             = 18,
	IPC_CMD9_EXCEPTION         = 19
} IpcCmd9;

typedef enum
{
	IPC_CMD11_PRINT_MSG = 0,
	IPC_CMD11_PANIC     = 1,
	IPC_CMD11_EXCEPTION = 2
} IpcCmd11;



u32 IPC_handleCmd(u8 cmd, UNUSED u8 params);
