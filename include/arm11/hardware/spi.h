#pragma once

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


// SPI_NEW_CNT
#define SPI_CNT_DIRE_READ   (0u)
#define SPI_CNT_DIRE_WRITE  (1u<<13)
#define SPI_CNT_ENABLE      (1u<<15)

// SPI_NEW_STATUS
#define SPI_STATUS_BUSY     (1u)


// TODO: Proper device table
typedef enum
{
	SPI_DEV_UNK1   = 0,
	SPI_DEV_NVRAM  = 1,
	SPI_DEV_UNK2   = 2,
	SPI_DEV_CODEC  = 3,
	SPI_DEV_UNK3   = 0,
	SPI_DEV_UNK4   = 0,
} SpiDevice;



/**
 * @brief      Initializes the SPI buses. Call this only once.
 */
void SPI_init(void);

/**
 * @brief      Writes and/or reads data to/from a SPI device.
 *
 * @param[in]  dev      The device ID. See table above.
 * @param[in]  in       Input data pointer for write.
 * @param      out      Output data pointer for read.
 * @param[in]  inSize   Input size.
 * @param[in]  outSize  Output size.
 * @param[in]  done     Set to true if this is the last transfer (chip select).
 */
void SPI_writeRead(SpiDevice dev, const u32 *in, u32 *out, u32 inSize, u32 outSize, bool done);
