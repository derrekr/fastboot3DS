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
#include "arm11/hardware/spiflash.h"
#include "arm11/hardware/spi.h"



bool spiflash_get_status(void)
{
	alignas(4) u8 cmdBuf[4];

	cmdBuf[0] = SPIFLASH_CMD_RDSR;
	SPI_writeRead(SPI_DEV_NVRAM, (u32*)cmdBuf, (u32*)cmdBuf, 1, 1, true);

	if(cmdBuf[0] & 1) return false;
	return true;
}

void spiflash_read(u32 offset, u32 size, u32 *buf)
{
	alignas(4) u8 cmdBuf[4];

	cmdBuf[0] = SPIFLASH_CMD_READ;
	cmdBuf[1] = (offset>>16) & 0xFFu;
	cmdBuf[2] = (offset>>8) & 0xFFu;
	cmdBuf[3] = offset & 0xFFu;
	SPI_writeRead(SPI_DEV_NVRAM, (u32*)cmdBuf, NULL, 4, 0, false);
	SPI_writeRead(SPI_DEV_NVRAM, NULL, buf, 0, size, true);
}
