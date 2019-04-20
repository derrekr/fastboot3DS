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



static void codecSelectReg(u8 reg)
{
	alignas(4) u8 buf[4];

	buf[0] = 0;
	buf[1] = reg;

	SPI_writeRead(SPI_DEV_CODEC, (u32*)buf, NULL, 2, 0, true);
}

static u8 codecReadReg(u8 offset)
{
	alignas(4) u8 buf[4];

	buf[0] = offset<<1 | 1u;
	SPI_writeRead(SPI_DEV_CODEC, (u32*)buf, (u32*)buf, 1, 1, true);

	return buf[0];
}

static void codecWriteReg(u8 reg, u8 val)
{
	alignas(4) u8 buf[4];

	buf[0] = reg<<1; // Write
	buf[1] = val;

	SPI_writeRead(SPI_DEV_CODEC, (u32*)buf, NULL, 2, 0, true);
}

static void codecReadRegBuf(u8 offset, u32 *buffer, u32 size)
{
	alignas(4) u8 buf[4];

	buf[0] = offset<<1 | 1u;
	SPI_writeRead(SPI_DEV_CODEC, (u32*)buf, buffer, 1, size, true);
}

static void codecMaskReg(u8 offset, u8 mask0, u8 mask1)
{
	alignas(4) u8 buf[4];

	buf[0] = offset<<1 | 1u;
	SPI_writeRead(SPI_DEV_CODEC, (u32*)buf, (u32*)buf, 1, 1, true);

	buf[1] = (buf[0] & ~mask1) | (mask0 & mask1);
	buf[0] = offset<<1;

	SPI_writeRead(SPI_DEV_CODEC, (u32*)buf, NULL, 2, 0, true);
}

void CODEC_init(void)
{
	codecSelectReg(0x67);
	codecWriteReg(0x24, 0x98);
	codecSelectReg(0x67);
	codecWriteReg(0x26, 0x00);
	codecSelectReg(0x67);
	codecWriteReg(0x25, 0x43);
	codecSelectReg(0x67);
	codecWriteReg(0x24, 0x18);
	codecSelectReg(0x67);
	codecWriteReg(0x17, 0x43);
	codecSelectReg(0x67);
	codecWriteReg(0x19, 0x69);
	codecSelectReg(0x67);
	codecWriteReg(0x1B, 0x80);
	codecSelectReg(0x67);
	codecWriteReg(0x27, 0x11);
	codecSelectReg(0x67);
	codecWriteReg(0x26, 0xEC);
	codecSelectReg(0x67);
	codecWriteReg(0x24, 0x18);
	codecSelectReg(0x67);
	codecWriteReg(0x25, 0x53);

	codecSelectReg(0x67);
	codecMaskReg(0x26, 0x80, 0x80);
	codecSelectReg(0x67);
	codecMaskReg(0x24, 0x00, 0x80);
	codecSelectReg(0x67);
	codecMaskReg(0x25, 0x10, 0x3C);
}

void CODEC_getRawData(u32 buf[13])
{
	//codecSelectReg(0x67);
	// This reg read seems useless and doesn't affect funtionality.
	//codecReadReg(0x26);

	codecSelectReg(0xFB);
	codecReadRegBuf(1, buf, 52);
}
