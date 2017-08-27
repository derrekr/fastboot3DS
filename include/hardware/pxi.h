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
#define PXI_DATA_RECEIVED(reg)      ((reg) & 0xFFu)
#define PXI_DATA_SENT(reg, sent)    (((reg) & ~(0xFFu<<8)) | sent<<8)
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


typedef enum
{
	PXI_CMD9_FMOUNT            = 0,
	PXI_CMD9_FUNMOUNT          = 1,
	PXI_CMD9_FOPEN             = 2,
	PXI_CMD9_FCLOSE            = 3,
	PXI_CMD9_FREAD             = 4,
	PXI_CMD9_FWRITE            = 5,
	PXI_CMD9_FOPEN_DIR         = 6,
	PXI_CMD9_FREAD_DIR         = 7,
	PXI_CMD9_FCLOSE_DIR        = 8,
	PXI_CMD9_FUNLINK           = 9,
	PXI_CMD9_FGETFREE          = 10,
	PXI_CMD9_READ_SECTORS      = 11,
	PXI_CMD9_WRITE_SECTORS     = 12,
	PXI_CMD9_MALLOC            = 13,
	PXI_CMD9_FREE              = 14,
	PXI_CMD9_LOAD_VERIFY_FIRM  = 15,
	PXI_CMD9_FIRM_LAUNCH       = 16,
	PXI_CMD9_PREPA_POWER       = 17,
	PXI_CMD9_PANIC             = 18,
	PXI_CMD9_EXCEPTION         = 19
} PxiCmd9;

typedef enum
{
	PXI_CMD11_PRINT_MSG = 0,
	PXI_CMD11_PANIC     = 1,
	PXI_CMD11_EXCEPTION = 2
} PxiCmd11;



void PXI_init(void);
u32 PXI_sendCmd(u32 cmd, const u32 *const buf, u8 words);
