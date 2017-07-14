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
#include "arm11/interrupt.h"
#include "gfx.h"
#include "arm11/main.h"
#include "arm11/power.h"



static void pxiIrqHandler(UNUSED u32 intSource);

void PXI_init(void)
{
	REG_PXI_SYNC11 = PXI_IRQ_ENABLE;
	REG_PXI_CNT11 = PXI_FLUSH_SEND_FIFO | PXI_EMPTY_FULL_ERROR | PXI_ENABLE_SEND_RECV_FIFO;

	while((REG_PXI_SYNC11 & 0xFFu) != 9u);
	REG_PXI_SYNC11 |= 11u<<8;

	IRQ_registerHandler(IRQ_PXI_SYNC, 13, 0, true, pxiIrqHandler);
}

static void pxiIrqHandler(UNUSED u32 intSource)
{
	u32 cmdCode = PXI_recvWord();

	switch(cmdCode)
	{
		case PXI_CMD_ENABLE_LCDS:
			gfx_init();
			PXI_sendWord(PXI_RPL_OK);
			break;
		case PXI_CMD_ALLOW_POWER_OFF:
			g_poweroffAllowed = true;
			break;
		case PXI_CMD_FORBID_POWER_OFF:
			g_poweroffAllowed = false;
			break;
		case PXI_CMD_POWER_OFF:
			power_off();
			break;
		case PXI_CMD_REBOOT:
			power_reboot();
			break;
		case PXI_CMD_FIRM_LAUNCH:
			g_startFirmLaunch = true;
			break;
		default: ;
	}
}

void PXI_sendWord(u32 val)
{
	while(REG_PXI_CNT11 & PXI_SEND_FIFO_FULL);
	REG_PXI_SEND11 = val;
	REG_PXI_SYNC11 |= PXI_NOTIFY_9;
}

bool PXI_trySendWord(u32 val)
{
	if(REG_PXI_CNT11 & PXI_SEND_FIFO_FULL)
		return false;
	REG_PXI_SEND11 = val;
	REG_PXI_SYNC11 |= PXI_NOTIFY_9;
	return true;
}

u32 PXI_recvWord(void)
{
	while(REG_PXI_CNT11 & PXI_RECV_FIFO_EMPTY);
	return REG_PXI_RECV11;
}

u32 PXI_tryRecvWord(bool *success)
{
	if(REG_PXI_CNT11 & PXI_RECV_FIFO_EMPTY)
	{
		*success = false;
		return 0;
	}

	*success = true;
	return REG_PXI_RECV11;
}
