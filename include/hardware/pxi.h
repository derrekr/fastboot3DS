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
#define PXI_REGS_BASE                (IO_MEM_ARM9_ONLY + 0x8000)
#elif ARM11
#define PXI_REGS_BASE                (IO_MEM_ARM9_ARM11 + 0x63000)
#endif
#define REG_PXI_DATA_RECEIVED        *((vu8 *)(PXI_REGS_BASE + 0x00))
#define REG_PXI_DATA_SENT            *((vu8 *)(PXI_REGS_BASE + 0x01))
#define REG_PXI_SYNC                 *((vu32*)(PXI_REGS_BASE + 0x00))
#define REG_PXI_CNT                  *((vu16*)(PXI_REGS_BASE + 0x04))
#define REG_PXI_SEND                 *((vu32*)(PXI_REGS_BASE + 0x08))
#define REG_PXI_RECV                 *((vu32*)(PXI_REGS_BASE + 0x0C))


// Defines for PX_SYNC regs
#define PXI_DATA_RECEIVED            (REG_PXI_SYNC & 0xFFu)
#define PXI_DATA_SENT(sent)          ((REG_PXI_SYNC & ~(0xFFu<<8)) | sent<<8)
#ifdef ARM9
#define PXI_TRIGGER_SYNC_IRQ         (1u<<29)
#elif ARM11
#define PXI_TRIGGER_SYNC_IRQ         (1u<<30)
#endif
#define PXI_IRQ_ENABLE               (1u<<31)

// Defines for PXI_CNT regs
#define PXI_SEND_FIFO_EMPTY          (1u<<0)
#define PXI_SEND_FIFO_FULL           (1u<<1)
#define PXI_SEND_FIFO_EMPTY_IRQ      (1u<<2)
#define PXI_FLUSH_SEND_FIFO          (1u<<3)
#define PXI_RECV_FIFO_EMPTY          (1u<<8)
#define PXI_RECV_FIFO_FULL           (1u<<9)
#define PXI_RECV_FIFO_NOT_EMPTY_IRQ  (1u<<10)
#define PXI_EMPTY_FULL_ERROR         (1u<<14)
#define PXI_ENABLE_SEND_RECV_FIFO    (1u<<15)



void PXI_init(void);
u32 PXI_sendCmd(u32 cmd, const u32 *const buf, u8 words);
