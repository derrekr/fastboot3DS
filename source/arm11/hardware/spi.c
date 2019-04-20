/*
 *   This file is part of fastboot 3DS
 *   Copyright (C) 2019 derrek, profi200
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
#include "mem_map.h"
#include "arm11/hardware/spi.h"
#include "arm11/hardware/cfg11.h"
#include "arm11/hardware/interrupt.h"


#define SPI1_REGS_BASE  (IO_MEM_ARM9_ARM11 + 0x42800)
#define SPI2_REGS_BASE  (IO_MEM_ARM9_ARM11 + 0x43800)
#define SPI3_REGS_BASE  (IO_MEM_ARM9_ARM11 + 0x60800)


typedef struct
{
	vu32 SPI_NEW_CNT;
	vu32 SPI_NEW_DONE;
	vu32 SPI_NEW_BLKLEN;
	vu32 SPI_NEW_FIFO;
	vu32 SPI_NEW_STATUS;
} SpiRegs;

enum
{
	SPI_BUS1 = 0,
	SPI_BUS2 = 1,
	SPI_BUS3 = 2
};

// TODO: Confirm these baudrates
enum
{
	SPI_BAUD_128KHz = 0,
	SPI_BAUD_256KHz = 1,
	SPI_BAUD_512KHz = 2,
	SPI_BAUD_1MHz   = 3,
	SPI_BAUD_2MHz   = 4,
	SPI_BAUD_4MHz   = 5,
};

static const struct
{
	u8 busId;
	u8 csBaud;
} spiDevTable[] =
{
	{SPI_BUS1, 0u<<6 | SPI_BAUD_512KHz},
	{SPI_BUS3, 1u<<6 | 0}, // TODO: Bus, baudrate
	{SPI_BUS1, 2u<<6 | 0}, // TODO: Bus, baudrate
	{SPI_BUS1, 0u<<6 | SPI_BAUD_4MHz},
	{SPI_BUS1, 1u<<6 | 0}, // TODO: Bus, baudrate
	{SPI_BUS1, 2u<<6 | 0}  // TODO: Bus, baudrate
};



static SpiRegs* spiGetBusRegsBase(u8 busId)
{
	SpiRegs *base;
	switch(busId)
	{
		case SPI_BUS1:
			base = (SpiRegs*)SPI1_REGS_BASE;
			break;
		case SPI_BUS2:
			base = (SpiRegs*)SPI2_REGS_BASE;
			break;
		case SPI_BUS3:
			base = (SpiRegs*)SPI3_REGS_BASE;
			break;
		default:
			base = NULL;
	}

	return base;
}

static void spiWaitBusy(const SpiRegs *const regs)
{
	while(regs->SPI_NEW_CNT & SPI_CNT_ENABLE);
}

static void spiWaitFifoBusy(const SpiRegs *const regs)
{
	while(regs->SPI_NEW_STATUS & SPI_STATUS_BUSY);
}

void SPI_init(void)
{
	// Switch all 3 buses to the new interface
	REG_CFG11_SPI_CNT = 1u<<2 | 1u<<1 | 1u;
}

void SPI_writeRead(SpiDevice dev, const u32 *in, u32 *out, u32 inSize, u32 outSize, bool done)
{
	SpiRegs *const regs = spiGetBusRegsBase(spiDevTable[dev].busId);
	const u32 cntParams = SPI_CNT_ENABLE | spiDevTable[dev].csBaud;

	if(in)
	{
		regs->SPI_NEW_BLKLEN = inSize;
		regs->SPI_NEW_CNT = cntParams | SPI_CNT_DIRE_WRITE;

		u32 counter = 0;
		do
		{
			if((counter & 31) == 0) spiWaitFifoBusy(regs);
			regs->SPI_NEW_FIFO = *in++;
			counter += 4;
		} while(counter < inSize);

		spiWaitBusy(regs);
	}
	if(out)
	{
		regs->SPI_NEW_BLKLEN = outSize;
		regs->SPI_NEW_CNT = cntParams | SPI_CNT_DIRE_READ;

		u32 counter = 0;
		do
		{
			if((counter & 31) == 0) spiWaitFifoBusy(regs);
			*out++ = regs->SPI_NEW_FIFO;
			counter += 4;
		} while(counter < outSize);

		spiWaitBusy(regs);
	}

	if(done) regs->SPI_NEW_DONE = 0;
}
