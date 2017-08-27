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
#include "ipc_handler.h"
#include "hardware/pxi.h"
//#include "arm11/debug.h"


// Temporary until we have a panic() function.
#define panic()  *((vu32*)4) = 0xDEADBEEF



u32 IPC_handleCmd(u8 cmd, UNUSED const u32 *buf)
{
	u32 result = 0;

	switch(cmd)
	{
		case IPC_CMD_MASK(IPC_CMD11_PRINT_MSG):
			break;
		case IPC_CMD_MASK(IPC_CMD11_PANIC):
			break;
		case IPC_CMD_MASK(IPC_CMD11_EXCEPTION):
			break;
		default:
			panic();
	}

	return result;
}
