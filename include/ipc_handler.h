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
#define IPC_CMD_RESP_FLAG           (1u<<15)
#define IPC_CMD_ID_MASK(cmd)        ((cmd)>>8)      // Max 127
#define IPC_CMD_IN_BUFS_MASK(cmd)   ((cmd)>>6 & 3u) // Max 3
#define IPC_CMD_OUT_BUFS_MASK(cmd)  ((cmd)>>4 & 3u) // Max 3
#define IPC_CMD_PARAMS_MASK(cmd)    ((cmd) & 15u)   // Max 15


#define MAKE_CMD(id, inBufs, outBufs, params)  ((id)<<8 | (inBufs)<<6 | (outBufs)<<4 | params)

typedef enum
{
	IPC_CMD9_FMOUNT              = MAKE_CMD(0, 0, 0, 1),
	IPC_CMD9_FUNMOUNT            = MAKE_CMD(1, 0, 0, 1),
	IPC_CMD9_FIS_DRIVE_MOUNTED   = MAKE_CMD(2, 0, 0, 1),
	IPC_CMD9_FGETFREE            = MAKE_CMD(3, 0, 1, 1),
	IPC_CMD9_FGET_DEV_SIZE       = MAKE_CMD(4, 0, 0, 1),
	IPC_CMD9_FIS_DEV_ACTIVE      = MAKE_CMD(5, 0, 0, 1),
	IPC_CMD9_FPREP_RAW_ACCESS    = MAKE_CMD(6, 0, 0, 1),
	IPC_CMD9_FFINAL_RAW_ACCESS   = MAKE_CMD(7, 0, 0, 1),
	IPC_CMD9_FCREATE_DEV_BUF     = MAKE_CMD(8, 0, 0, 1),
	IPC_CMD9_FFREE_DEV_BUF       = MAKE_CMD(9, 0, 0, 1),
	IPC_CMD9_FREAD_TO_DEV_BUF    = MAKE_CMD(10, 0, 0, 4),
	IPC_CMD9_FWRITE_FROM_DEV_BUF = MAKE_CMD(11, 0, 0, 4),
	IPC_CMD9_FOPEN               = MAKE_CMD(12, 1, 0, 1),
	IPC_CMD9_FREAD               = MAKE_CMD(13, 0, 1, 1),
	IPC_CMD9_FWRITE              = MAKE_CMD(14, 1, 0, 1),
	IPC_CMD9_FSYNC               = MAKE_CMD(15, 0, 0, 1),
	IPC_CMD9_FLSEEK              = MAKE_CMD(16, 0, 0, 2),
	IPC_CMD9_FTELL               = MAKE_CMD(17, 0, 0, 1),
	IPC_CMD9_FSIZE               = MAKE_CMD(18, 0, 0, 1),
	IPC_CMD9_FCLOSE              = MAKE_CMD(19, 0, 0, 1),
	IPC_CMD9_FEXPAND             = MAKE_CMD(20, 0, 0, 2),
	IPC_CMD9_FSTAT               = MAKE_CMD(21, 1, 1, 0),
	IPC_CMD9_FOPEN_DIR           = MAKE_CMD(22, 1, 0, 0),
	IPC_CMD9_FREAD_DIR           = MAKE_CMD(23, 0, 1, 2),
	IPC_CMD9_FCLOSE_DIR          = MAKE_CMD(24, 0, 0, 1),
	IPC_CMD9_FMKDIR              = MAKE_CMD(25, 1, 0, 0),
	IPC_CMD9_FRENAME             = MAKE_CMD(26, 2, 0, 0),
	IPC_CMD9_FUNLINK             = MAKE_CMD(27, 1, 0, 0),
	IPC_CMD9_FVERIFY_NAND_IMG    = MAKE_CMD(28, 1, 0, 0),
	IPC_CMD9_FSET_NAND_PROT      = MAKE_CMD(29, 0, 0, 1),
	IPC_CMD9_WRITE_FIRM_PART     = MAKE_CMD(30, 1, 0, 1),
	IPC_CMD9_LOAD_VERIFY_FIRM    = MAKE_CMD(31, 1, 0, 1),
	IPC_CMD9_FIRM_LAUNCH         = MAKE_CMD(32, 0, 0, 0),
	IPC_CMD9_LOAD_VERIFY_UPDATE  = MAKE_CMD(33, 1, 1, 0),
	IPC_CMD9_GET_BOOT_ENV        = MAKE_CMD(34, 0, 0, 0),
	IPC_CMD9_TOGGLE_SUPERHAX     = MAKE_CMD(35, 0, 0, 1),
	IPC_CMD9_PREPARE_POWER       = MAKE_CMD(36, 0, 0, 0),
	IPC_CMD9_PANIC               = MAKE_CMD(37, 0, 0, 0),
	IPC_CMD9_EXCEPTION           = MAKE_CMD(38, 0, 0, 0)
} IpcCmd9;

typedef enum
{
	IPC_CMD11_PRINT_MSG        = MAKE_CMD(0, 0, 0, 0), // Invalid on purpose. Will be decided later.
	IPC_CMD11_PANIC            = MAKE_CMD(1, 0, 0, 0),
	IPC_CMD11_EXCEPTION        = MAKE_CMD(2, 0, 0, 0)
} IpcCmd11;

#undef MAKE_CMD


typedef struct
{
	void *ptr;
	u32 size;
} IpcBuffer;



u32 IPC_handleCmd(u8 cmdId, u32 inBufs, u32 outBufs, const u32 *const buf);
