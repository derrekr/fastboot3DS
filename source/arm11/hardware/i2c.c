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
#include "mem_map.h"
#include "arm11/hardware/i2c.h"
#include "arm11/hardware/interrupt.h"


#define I2C1_REGS_BASE  (IO_MEM_ARM9_ARM11 + 0x61000)
#define I2C2_REGS_BASE  (IO_MEM_ARM9_ARM11 + 0x44000)
#define I2C3_REGS_BASE  (IO_MEM_ARM9_ARM11 + 0x48000)


typedef struct
{
	vu8  I2C_DATA;
	vu8  I2C_CNT;
	vu16 I2C_CNTEX;
	vu16 I2C_SCL;
} I2cRegs;

enum
{
	I2C_BUS1 = 0,
	I2C_BUS2 = 1,
	I2C_BUS3 = 2
};

static const struct
{
	u8 busId;
	u8 devAddr;
} i2cDevTable[] =
{
	{I2C_BUS1, 0x4A},
	{I2C_BUS1, 0x7A},
	{I2C_BUS1, 0x78},
	{I2C_BUS2, 0x4A},
	{I2C_BUS2, 0x78},
	{I2C_BUS2, 0x2C},
	{I2C_BUS2, 0x2E},
	{I2C_BUS2, 0x40},
	{I2C_BUS2, 0x44},
	{I2C_BUS3, 0xD6},
	{I2C_BUS3, 0xD0},
	{I2C_BUS3, 0xD2},
	{I2C_BUS3, 0xA4},
	{I2C_BUS3, 0x9A},
	{I2C_BUS3, 0xA0},
	{I2C_BUS2, 0xEE},
	{I2C_BUS1, 0x40},
	{I2C_BUS3, 0x54}
};



static I2cRegs* i2cGetBusRegsBase(u8 busId)
{
	I2cRegs *base;
	switch(busId)
	{
		case I2C_BUS1:
			base = (I2cRegs*)I2C1_REGS_BASE;
			break;
		case I2C_BUS2:
			base = (I2cRegs*)I2C2_REGS_BASE;
			break;
		case I2C_BUS3:
			base = (I2cRegs*)I2C3_REGS_BASE;
			break;
		default:
			base = NULL;
	}

	return base;
}

static inline void i2cWaitBusyIrq(const vu8 *const I2C_CNT)
{
	do
	{
		__wfi();
	} while(*I2C_CNT & I2C_ENABLE);
}

static bool i2cCheckAck(vu8 *const I2C_CNT)
{
	if(!(*I2C_CNT & I2C_ACK)) // If ack flag is 0 it failed.
	{
		*I2C_CNT = I2C_ENABLE | I2C_IRQ_ENABLE | I2C_ERROR | I2C_STOP;
		return false;
	}

	return true;
}

void I2C_init(void)
{
	static bool inited = false;
	if(inited) return;
	inited = true;

	IRQ_registerHandler(IRQ_I2C1, 14, 0, true, NULL);
	IRQ_registerHandler(IRQ_I2C2, 14, 0, true, NULL);
	IRQ_registerHandler(IRQ_I2C3, 14, 0, true, NULL);

	I2cRegs *regs = i2cGetBusRegsBase(I2C_BUS1);
	while(regs->I2C_CNT & I2C_ENABLE);
	regs->I2C_CNTEX = 2;  // TODO: Find out what this does. If bit 1 is not set I2C hangs.
	regs->I2C_SCL = 1280; // TODO: How to calculate the frequency.

	regs = i2cGetBusRegsBase(I2C_BUS2);
	while(regs->I2C_CNT & I2C_ENABLE);
	regs->I2C_CNTEX = 2;
	regs->I2C_SCL = 1280;

	regs = i2cGetBusRegsBase(I2C_BUS3);
	while(regs->I2C_CNT & I2C_ENABLE);
	regs->I2C_CNTEX = 2;
	regs->I2C_SCL = 1280;
}

static bool i2cStartTransfer(u8 devAddr, u8 regAddr, bool read, I2cRegs *const regs)
{
	u32 tries = 8;
	do
	{
		// This is a special case where we can't predict when or if
		// the IRQ has already fired. If it fires after checking but
		// before a wfi this would hang.
		while(regs->I2C_CNT & I2C_ENABLE) __wfe();

		// Select device and start.
		regs->I2C_DATA = devAddr;
		regs->I2C_CNT = I2C_ENABLE | I2C_IRQ_ENABLE | I2C_DIRE_WRITE | I2C_START;
		i2cWaitBusyIrq(&regs->I2C_CNT);
		if(!i2cCheckAck(&regs->I2C_CNT)) continue;

		// Select register.
		regs->I2C_DATA = regAddr;
		regs->I2C_CNT = I2C_ENABLE | I2C_IRQ_ENABLE | I2C_DIRE_WRITE;
		i2cWaitBusyIrq(&regs->I2C_CNT);
		if(!i2cCheckAck(&regs->I2C_CNT)) continue;

		// Select device in read mode for read transfer.
		if(read)
		{
			regs->I2C_DATA = devAddr | 1u; // Set bit 0 for read.
			regs->I2C_CNT = I2C_ENABLE | I2C_IRQ_ENABLE | I2C_DIRE_WRITE | I2C_START;
			i2cWaitBusyIrq(&regs->I2C_CNT);
			if(!i2cCheckAck(&regs->I2C_CNT)) continue;
		}

		break;
	} while(--tries > 0);

	return tries > 0;
}

bool I2C_readRegBuf(I2cDevice devId, u8 regAddr, u8 *out, u32 size)
{
	const u8 devAddr = i2cDevTable[devId].devAddr;
	I2cRegs *const regs = i2cGetBusRegsBase(i2cDevTable[devId].busId);


	if(!i2cStartTransfer(devAddr, regAddr, true, regs)) return false;

	while(--size)
	{
		regs->I2C_CNT = I2C_ENABLE | I2C_IRQ_ENABLE | I2C_DIRE_READ | I2C_ACK;
		i2cWaitBusyIrq(&regs->I2C_CNT);
		*out++ = regs->I2C_DATA;
	}

	// Last byte transfer.
	regs->I2C_CNT = I2C_ENABLE | I2C_IRQ_ENABLE | I2C_DIRE_READ | I2C_STOP;
	i2cWaitBusyIrq(&regs->I2C_CNT);
	*out = regs->I2C_DATA;

	return true;
}

bool I2C_writeRegBuf(I2cDevice devId, u8 regAddr, const u8 *in, u32 size)
{
	const u8 devAddr = i2cDevTable[devId].devAddr;
	I2cRegs *const regs = i2cGetBusRegsBase(i2cDevTable[devId].busId);


	if(!i2cStartTransfer(devAddr, regAddr, false, regs)) return false;

	while(--size)
	{
		regs->I2C_DATA = *in++;
		regs->I2C_CNT = I2C_ENABLE | I2C_IRQ_ENABLE | I2C_DIRE_WRITE;
		i2cWaitBusyIrq(&regs->I2C_CNT);
		if(!i2cCheckAck(&regs->I2C_CNT)) return false;
	}

	// Last byte transfer.
	regs->I2C_DATA = *in;
	regs->I2C_CNT = I2C_ENABLE | I2C_IRQ_ENABLE | I2C_DIRE_WRITE | I2C_STOP;
	i2cWaitBusyIrq(&regs->I2C_CNT);
	if(!i2cCheckAck(&regs->I2C_CNT)) return false;

	return true;
}

u8 I2C_readReg(I2cDevice devId, u8 regAddr)
{
	u8 data;
	if(!I2C_readRegBuf(devId, regAddr, &data, 1)) return 0xFF;
	return data;
}

bool I2C_writeReg(I2cDevice devId, u8 regAddr, u8 data)
{
	return I2C_writeRegBuf(devId, regAddr, &data, 1);
}

bool I2C_writeRegIntSafe(I2cDevice devId, u8 regAddr, u8 data)
{
	const u8 devAddr = i2cDevTable[devId].devAddr;
	I2cRegs *const regs = i2cGetBusRegsBase(i2cDevTable[devId].busId);


	u32 tries = 8;
	do
	{
		while(regs->I2C_CNT & I2C_ENABLE);

		// Select device and start.
		regs->I2C_DATA = devAddr;
		regs->I2C_CNT = I2C_ENABLE | I2C_DIRE_WRITE | I2C_START;
		while(regs->I2C_CNT & I2C_ENABLE);
		if(!i2cCheckAck(&regs->I2C_CNT)) continue;

		// Select register.
		regs->I2C_DATA = regAddr;
		regs->I2C_CNT = I2C_ENABLE | I2C_DIRE_WRITE;
		while(regs->I2C_CNT & I2C_ENABLE);
		if(!i2cCheckAck(&regs->I2C_CNT)) continue;

		break;
	} while(--tries > 0);

	if(tries == 0) return false;

	regs->I2C_DATA = data;
	regs->I2C_CNT = I2C_ENABLE | I2C_DIRE_WRITE | I2C_STOP;
	while(regs->I2C_CNT & I2C_ENABLE);
	if(!i2cCheckAck(&regs->I2C_CNT)) return false;

	return true;
}
