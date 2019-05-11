/*
 *   This file is part of fastboot 3DS
 *   Copyright (C) 2019 Sergi Granell (xerpi), Paul LaMendola (paulguy), derrek, profi200
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

// Based on code from https://github.com/xerpi/linux_3ds/blob/master/drivers/input/misc/nintendo3ds_codec_hid.c

#include "types.h"
#include "arm11/hardware/spi.h"



static void codecReadRegBuf(u8 reg, u32 *buf, u32 size)
{
	alignas(4) u8 inBuf[4];

	inBuf[0] = reg<<1 | 1u;
	NSPI_writeRead(SPI_DEV_CODEC, (u32*)inBuf, buf, 1, size, true);
}

static u8 codecReadReg(u8 reg)
{
	alignas(4) u8 outBuf[4];

	codecReadRegBuf(reg, (u32*)outBuf, 1);

	return outBuf[0];
}

static void codecWriteReg(u8 reg, u8 val)
{
	alignas(4) u8 inBuf[4];

	inBuf[0] = reg<<1; // Write
	inBuf[1] = val;

	NSPI_writeRead(SPI_DEV_CODEC, (u32*)inBuf, NULL, 2, 0, true);
}

static void codecMaskReg(u8 reg, u8 mask, u8 val)
{
	u8 data = codecReadReg(reg);
	data = (data & ~mask) | (val & mask);
	codecWriteReg(reg, data);
}

static void codecSwitchBank(u8 bank)
{
	static u8 curBank = 0;
	if(bank != curBank)
	{
		codecWriteReg(0, bank);
		curBank = bank;
	}
}

void CODEC_init(void)
{
	static bool inited = false;
	if(inited) return;
	inited = true;

	NSPI_init();

	codecSwitchBank(0x67);
	codecWriteReg(0x24, 0x98);
	codecWriteReg(0x26, 0x00);
	codecWriteReg(0x25, 0x43);
	codecWriteReg(0x24, 0x18);
	codecWriteReg(0x17, 0x43);
	codecWriteReg(0x19, 0x69);
	codecWriteReg(0x1B, 0x80);
	codecWriteReg(0x27, 0x11);
	codecWriteReg(0x26, 0xEC);
	codecWriteReg(0x24, 0x18);
	codecWriteReg(0x25, 0x53);

	codecMaskReg(0x26, 0x80, 0x80);
	codecMaskReg(0x24, 0x80, 0x00);
	codecMaskReg(0x25, 0x3C, 0x10);
}

void CODEC_getRawData(u32 buf[13])
{
	//codecSwitchBank(0x67);
	// This reg read seems useless and doesn't affect funtionality.
	//codecReadReg(0x26);

	codecSwitchBank(0xFB);
	codecReadRegBuf(1, buf, 52);
}
