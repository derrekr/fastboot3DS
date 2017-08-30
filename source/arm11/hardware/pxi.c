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
#include "hardware/pxi.h"
#include "arm11/hardware/interrupt.h"
//#include "arm11/debug.h"
#include "fb_assert.h"
#include "ipc_handler.h"


// Temporary until we have a panic() function.
#define panic()  *((vu32*)4) = 0xDEADBEEF



static void pxiIrqHandler(UNUSED u32 intSource);

void PXI_init(void)
{
	REG_PXI_SYNC11 = PXI_IRQ_ENABLE;
	REG_PXI_CNT11 = PXI_FLUSH_SEND_FIFO | PXI_EMPTY_FULL_ERROR | PXI_ENABLE_SEND_RECV_FIFO;

	while(PXI_DATA_RECEIVED(REG_PXI_SYNC11) != 9u);
	REG_PXI_SYNC11 |= 11u<<8;

	IRQ_registerHandler(IRQ_PXI_SYNC, 13, 0, true, pxiIrqHandler);
}

static void pxiIrqHandler(UNUSED u32 intSource)
{
	const u32 cmdCode = REG_PXI_RECV11;
	const u8 inBufs = IPC_CMD_IN_BUFS_MASK(cmdCode);
	const u8 outBufs = IPC_CMD_OUT_BUFS_MASK(cmdCode);
	const u8 params = IPC_CMD_PARAMS_MASK(cmdCode);
	const u32 cmdBufSize = (inBufs * 2) + (outBufs * 2) + params;

	if(cmdBufSize > IPC_MAX_PARAMS || params != PXI_DATA_RECEIVED(REG_PXI_SYNC11))
	{
		panic();
	}

	u32 buf[IPC_MAX_PARAMS];
	for(u32 i = 0; i < cmdBufSize; i++) buf[i] = REG_PXI_RECV11;
	if(REG_PXI_SYNC11 & PXI_EMPTY_FULL_ERROR) panic();

	REG_PXI_SEND11 = IPC_handleCmd(IPC_CMD_ID_MASK(cmdCode), inBufs, outBufs, buf);
}

u32 PXI_sendCmd(u32 cmd, const u32 *const buf, u8 words)
{
	if(!buf) words = 0;
	fb_assert(words <= IPC_MAX_PARAMS);

	while(REG_PXI_CNT11 & PXI_SEND_FIFO_FULL);
	REG_PXI_SEND11 = cmd;
	for(u32 i = 0; i < words; i++) REG_PXI_SEND11 = buf[i];
	if(REG_PXI_SYNC11 & PXI_EMPTY_FULL_ERROR) panic();

	REG_PXI_SYNC11 = PXI_DATA_SENT(REG_PXI_SYNC11, words) | PXI_NOTIFY_9;

	while(REG_PXI_CNT11 & PXI_RECV_FIFO_EMPTY);
	return REG_PXI_RECV11;
}
