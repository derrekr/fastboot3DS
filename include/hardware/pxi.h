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

#include "mem_map.h"
#include "types.h"


#ifdef ARM9
#define PXI9_REGS_BASE              (IO_MEM_ARM9_ONLY + 0x8000)
#define REG_PXI_SYNC9               *((vu32*)(PXI9_REGS_BASE + 0x00))
#define REG_PXI_CNT9                *((vu32*)(PXI9_REGS_BASE + 0x04))
#define REG_PXI_SEND9               *((vu32*)(PXI9_REGS_BASE + 0x08))
#define REG_PXI_RECV9               *((vu32*)(PXI9_REGS_BASE + 0x0C))
#endif

#ifdef ARM11
#define PXI11_REGS_BASE             (IO_MEM_ARM9_ARM11 + 0x63000)
#define REG_PXI_SYNC11              *((vu32*)(PXI11_REGS_BASE + 0x00))
#define REG_PXI_CNT11               *((vu32*)(PXI11_REGS_BASE + 0x04))
#define REG_PXI_SEND11              *((vu32*)(PXI11_REGS_BASE + 0x08))
#define REG_PXI_RECV11              *((vu32*)(PXI11_REGS_BASE + 0x0C))
#endif


// Defines for PX_SYNC regs
#define PXI_DATA_RECEIVED(reg)      (reg & 0xFF)
#define PXI_DATA_SENT(reg)          (reg>>8 & 0xFF)
#define PXI_NOTIFY_11               (1u<<29)
#define PXI_NOTIFY_9                (1u<<30)
#define PXI_IRQ_ENABLE              (1u<<31)

// Defines for PX_CNT regs
#define PXI_SEND_FIFO_EMPTY         (1u<<0)
#define PXI_SEND_FIFO_FULL          (1u<<1)
#define PXI_SEND_FIFO_EMPTY_IRQ     (1u<<2)
#define PXI_FLUSH_SEND_FIFO         (1u<<3)
#define PXI_RECV_FIFO_EMPTY         (1u<<8)
#define PXI_RECV_FIFO_FULL          (1u<<9)
#define PXI_RECV_FIFO_NOT_EMPTY_IRQ (1u<<10)
#define PXI_EMPTY_FULL_ERROR        (1u<<14)
#define PXI_ENABLE_SEND_RECV_FIFO   (1u<<15)

// Custom PXI Command/Reply Definitions
#define PXI_CMD_ENABLE_LCDS        (0x4C434453)
#define PXI_CMD_SET_BRIGHTNESS     (0x78D28107)
#define PXI_CMD_POWER_OFF          (0x504F4646)
#define PXI_CMD_REBOOT             (0x48326482)
#define PXI_CMD_ALLOW_POWER_OFF    (0x504F4F4B)
#define PXI_CMD_FORBID_POWER_OFF   (0x504F4E4F)
#define PXI_CMD_FIRM_LAUNCH        (0x544F4F42)
#define PXI_RPL_FIRM_LAUNCH_READY  (0x4F4B6666)
#define PXI_RPL_OK                 (0x4F4B4F4B)
#define PXI_RPL_HOME_PRESSED       (0x484F4D45)
#define PXI_RPL_HOME_HELD          (0x484F4D22)
#define PXI_RPL_POWER_PRESSED      (0x504F5752)


void PXI_init(void);
void PXI_sendWord(u32 val);
bool PXI_trySendWord(u32 val);
u32  PXI_recvWord(void);
u32 PXI_tryRecvWord(bool *success);
void PXI_sendBuf(const u32 *const buf, u32 size);
