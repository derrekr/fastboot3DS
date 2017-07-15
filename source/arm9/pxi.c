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
#include "pxi.h"
#include "arm9/interrupt.h"



void PXI_init(void)
{
	REG_PXI_SYNC9 = PXI_IRQ_ENABLE;
	REG_PXI_CNT9 = PXI_FLUSH_SEND_FIFO | PXI_EMPTY_FULL_ERROR | PXI_ENABLE_SEND_RECV_FIFO;

	REG_PXI_SYNC9 |= 9u<<8;
	while((REG_PXI_SYNC9 & 0xFFu) != 11u);

	IRQ_registerHandler(IRQ_PXI_SYNC, NULL);
}

void PXI_sendWord(u32 val)
{
	while(REG_PXI_CNT9 & PXI_SEND_FIFO_FULL);
	REG_PXI_SEND9 = val;
	REG_PXI_SYNC9 |= PXI_NOTIFY_11;
}

bool PXI_trySendWord(u32 val)
{
	if(REG_PXI_CNT9 & PXI_SEND_FIFO_FULL)
		return false;
	REG_PXI_SEND9 = val;
	REG_PXI_SYNC9 |= PXI_NOTIFY_11;
	return true;
}

u32 PXI_recvWord(void)
{
	while(REG_PXI_CNT9 & PXI_RECV_FIFO_EMPTY);
	return REG_PXI_RECV9;
}

u32 PXI_tryRecvWord(bool *success)
{
	if(REG_PXI_CNT9 & PXI_RECV_FIFO_EMPTY)
	{
		*success = false;
		return 0;
	}

	*success = true;
	return REG_PXI_RECV9;
}

void PXI_sendBuf(const u32 *const buf, u32 size)
{
	while(REG_PXI_CNT9 & PXI_SEND_FIFO_FULL);
	for(u32 i = 0; i < size / 4; i++)
	{
		REG_PXI_SEND9 = buf[i];
	}
	REG_PXI_SYNC9 |= PXI_NOTIFY_11;
}
