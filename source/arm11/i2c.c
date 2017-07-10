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
#include "arm11/i2c.h"
#include "arm11/interrupt.h"


#define I2C1_REGS_BASE  (IO_MEM_ARM9_ARM11 + 0x61000)
#define REG_I2C1_DATA   *((vu8* )(I2C1_REGS_BASE + 0x00))
#define REG_I2C1_CNT    *((vu8* )(I2C1_REGS_BASE + 0x01))
#define REG_I2C1_CNTEX  *((vu16*)(I2C1_REGS_BASE + 0x02))
#define REG_I2C1_SCL    *((vu16*)(I2C1_REGS_BASE + 0x04))

#define I2C2_REGS_BASE  (IO_MEM_ARM9_ARM11 + 0x44000)
#define REG_I2C2_DATA   *((vu8* )(I2C2_REGS_BASE + 0x00))
#define REG_I2C2_CNT    *((vu8* )(I2C2_REGS_BASE + 0x01))
#define REG_I2C2_CNTEX  *((vu16*)(I2C2_REGS_BASE + 0x02))
#define REG_I2C2_SCL    *((vu16*)(I2C2_REGS_BASE + 0x04))

#define I2C3_REGS_BASE  (IO_MEM_ARM9_ARM11 + 0x48000)
#define REG_I2C3_DATA   *((vu8* )(I2C3_REGS_BASE + 0x00))
#define REG_I2C3_CNT    *((vu8* )(I2C3_REGS_BASE + 0x01))
#define REG_I2C3_CNTEX  *((vu16*)(I2C3_REGS_BASE + 0x02))
#define REG_I2C3_SCL    *((vu16*)(I2C3_REGS_BASE + 0x04))


static const struct
{
	u8 busId;
	u8 devAddr;
} i2cDevTable[] =
{
	{0,	0x4A},
	{0,	0x7A},
	{0,	0x78},
	{1,	0x4A},
	{1,	0x78},
	{1,	0x2C},
	{1,	0x2E},
	{1,	0x40},
	{1,	0x44},
	{2,	0xA6}, // TODO: Find out if 0xA6 or 0xD6 is correct
	{2,	0xD0},
	{2,	0xD2},
	{2,	0xA4},
	{2,	0x9A},
	{2,	0xA0},
	{1,	0xEE},
	{0,	0x40},
	{2,	0x54}
};



static void i2cWaitBusy(vu8 *cntReg)
{
	while(*cntReg & I2C_ENABLE);
}

static vu8* i2cGetBusRegsBase(u8 busId)
{
	vu8 *base;
	if(!busId)          base = (vu8*)I2C1_REGS_BASE;
	else if(busId == 1) base = (vu8*)I2C2_REGS_BASE;
	else                base = (vu8*)I2C3_REGS_BASE;

	return base;
}

static bool i2cStartTransfer(I2cDevice devId, u8 regAddr, bool read, vu8 *regsBase)
{
	const u8 devAddr = i2cDevTable[devId].devAddr;
	vu8 *const i2cData = regsBase;
	vu8 *const i2cCnt  = regsBase + 1;


	u32 i = 0;
	for(; i < 8; i++)
	{
		i2cWaitBusy(i2cCnt);

		// Select device and start.
		*i2cData = devAddr;
		*i2cCnt = I2C_ENABLE | I2C_IRQ_ENABLE | I2C_START;
		i2cWaitBusy(i2cCnt);
		if(!I2C_GET_ACK(*i2cCnt)) // If ack flag is 0 it failed.
		{
			*i2cCnt = I2C_ENABLE | I2C_IRQ_ENABLE | I2C_ERROR | I2C_STOP;
			continue;
		}

		// Select register and change direction to write.
		*i2cData = regAddr;
		*i2cCnt = I2C_ENABLE | I2C_IRQ_ENABLE | I2C_DIRE_WRITE;
		i2cWaitBusy(i2cCnt);
		if(!I2C_GET_ACK(*i2cCnt)) // If ack flag is 0 it failed.
		{
			*i2cCnt = I2C_ENABLE | I2C_IRQ_ENABLE | I2C_ERROR | I2C_STOP;
			continue;
		}

		// Select device in read mode for read transfer.
		if(read)
		{
			*i2cData = devAddr | 1u; // Set bit 0 for read.
			*i2cCnt = I2C_ENABLE | I2C_IRQ_ENABLE | I2C_START;
			i2cWaitBusy(i2cCnt);
			if(!I2C_GET_ACK(*i2cCnt)) // If ack flag is 0 it failed.
			{
				*i2cCnt = I2C_ENABLE | I2C_IRQ_ENABLE | I2C_ERROR | I2C_STOP;
				continue;
			}
		}

		break;
	}

	if(i < 8) return true;
	else return false;
}

void I2C_init(void)
{
	i2cWaitBusy(i2cGetBusRegsBase(0));
	REG_I2C1_CNTEX = 2;  // ?
	REG_I2C1_SCL = 1280; // ?

	i2cWaitBusy(i2cGetBusRegsBase(1));
	REG_I2C2_CNTEX = 2;  // ?
	REG_I2C2_SCL = 1280; // ?

	i2cWaitBusy(i2cGetBusRegsBase(2));
	REG_I2C3_CNTEX = 2;  // ?
	REG_I2C3_SCL = 1280; // ?
}

bool I2C_readRegBuf(I2cDevice devId, u8 regAddr, u8 *out, u32 size)
{
	enterCriticalSection(); // TODO: Instead of blocking other interrupts we need locks.
	const u8 busId = i2cDevTable[devId].busId;
	vu8 *const i2cData = i2cGetBusRegsBase(busId);
	vu8 *const i2cCnt  = i2cData + 1;


	if(!i2cStartTransfer(devId, regAddr, true, i2cData)) return false;

	while(--size)
	{
		*i2cCnt = I2C_ENABLE | I2C_IRQ_ENABLE | I2C_DIRE_READ | I2C_ACK;
		i2cWaitBusy(i2cCnt);
		*out++ = *i2cData;
	}

	*i2cCnt = I2C_ENABLE | I2C_IRQ_ENABLE | I2C_DIRE_READ | I2C_STOP;
	i2cWaitBusy(i2cCnt);
	*out = *i2cData; // Last byte

	leaveCriticalSection();
	return true;
}

bool I2C_writeRegBuf(I2cDevice devId, u8 regAddr, const u8 *in, u32 size)
{
	enterCriticalSection(); // TODO: Instead of blocking other interrupts we need locks.
	const u8 busId = i2cDevTable[devId].busId;
	vu8 *const i2cData = i2cGetBusRegsBase(busId);
	vu8 *const i2cCnt  = i2cData + 1;


	if(!i2cStartTransfer(devId, regAddr, false, i2cData)) return false;

	while(--size)
	{
		*i2cData = *in++;
		*i2cCnt = I2C_ENABLE | I2C_IRQ_ENABLE | I2C_DIRE_WRITE;
		i2cWaitBusy(i2cCnt);
		if(!I2C_GET_ACK(*i2cCnt)) // If ack flag is 0 it failed.
		{
			*i2cCnt = I2C_ENABLE | I2C_IRQ_ENABLE | I2C_ERROR | I2C_STOP;
			leaveCriticalSection();
			return false;
		}
	}

	*i2cData = *in;
	*i2cCnt = I2C_ENABLE | I2C_IRQ_ENABLE | I2C_DIRE_WRITE | I2C_STOP;
	i2cWaitBusy(i2cCnt);
	if(!I2C_GET_ACK(*i2cCnt)) // If ack flag is 0 it failed.
	{
		*i2cCnt = I2C_ENABLE | I2C_IRQ_ENABLE | I2C_ERROR | I2C_STOP;
		leaveCriticalSection();
		return false;
	}

	leaveCriticalSection();
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

u8 i2cmcu_readreg_hid_irq(void)
{
	u8 data;
	if(!I2C_readRegBuf(I2C_DEV_MCU, 0x10, &data, 1)) return 0;
	return data;
}

u8 i2cmcu_readreg_hid_held(void)
{
	u8 data[19];
	if(!I2C_readRegBuf(I2C_DEV_MCU, 0x7F, data, sizeof(data))) return 0;
	return data[18];
}

// aka i2cmcu_write_reg0x20_0x22
bool i2cmcu_lcd_poweron(void)
{
	return I2C_writeReg(I2C_DEV_MCU, 0x22, 2); // bit1 = lcd power enable for both screens
}

// aka i2cmcu_write_reg0x20_0x22_2
bool i2cmcu_lcd_backlight_poweron(void)
{
	return I2C_writeReg(I2C_DEV_MCU, 0x22, 0x28); // bit3 = lower screen, bit5 = upper
}

bool i2cmcu_lcd_poweroff(void)
{
	return I2C_writeReg(I2C_DEV_MCU, 0x22, 1); // bit0 = lcd power disable for both screens (also disabled backlight)
}
