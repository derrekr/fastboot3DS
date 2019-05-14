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


// REG_NSPI_CNT
#define NSPI_CNT_BUS_1BIT    (0u)
#define NSPI_CNT_BUS_4BIT    (1u<<12)
#define NSPI_CNT_DIRE_READ   (0u)
#define NSPI_CNT_DIRE_WRITE  (1u<<13)
#define NSPI_CNT_ENABLE      (1u<<15)

// REG_NSPI_DONE
#define NSPI_DONE_DONE       (0u)

// REG_NSPI_STATUS
#define NSPI_STATUS_BUSY     (1u)

// REG_NSPI_AUTOPOLL
#define NSPI_AUTOPOLL_START  (1u<<31)

// REG_NSPI_INT_MASK Bit set = disabled.
// REG_NSPI_INT_STAT Status and aknowledge.
#define NSPI_INT_UNK         (1u)    // Fires on auto poll success aswell. Transfer finished?
#define NSPI_INT_AP_SUCCESS  (1u<<1) // Auto poll
#define NSPI_INT_AP_TIMEOUT  (1u<<2) // Auto poll


// TODO: Confirm these clocks
enum
{
	NSPI_CLK_128KHz = 0,
	NSPI_CLK_256KHz = 1,
	NSPI_CLK_512KHz = 2,
	NSPI_CLK_1MHz   = 3,
	NSPI_CLK_2MHz   = 4,
	NSPI_CLK_4MHz   = 5,
};

// TODO: Proper device table
typedef enum
{
	SPI_DEV_UNK1   = 0,
	SPI_DEV_NVRAM  = 1,
	SPI_DEV_UNK3   = 2,
	SPI_DEV_CODEC  = 3,
	SPI_DEV_UNK5   = 4,
	SPI_DEV_UNK6   = 5,
	SPI_DEV_UNK7   = 6  // Don't use autopoll on this.
} SpiDevice;



/**
 * @brief      Initializes the SPI buses. Call this only once.
 */
void NSPI_init(void);

/**
 * @brief      Automatically polls a bit of the command response. Use with the macro below.
 *
 * @param[in]  dev     The device ID. See table above.
 * @param[in]  params  The parameters. Use the macro below.
 *
 * @return     Returns false on failure/timeout and true on success.
 */
bool _NSPI_autoPollBit(SpiDevice dev, u32 params);

/**
 * @brief      Writes and/or reads data to/from a SPI device.
 *
 * @param[in]  dev      The device ID. See table above.
 * @param[in]  in       Input data pointer for write.
 * @param      out      Output data pointer for read.
 * @param[in]  inSize   Input size. Must be <= 0x1FFFFF.
 * @param[in]  outSize  Output size. Must be <= 0x1FFFFF.
 * @param[in]  done     Set to true if this is the last transfer (chip select).
 */
void NSPI_writeRead(SpiDevice dev, const u32 *in, u32 *out, u32 inSize, u32 outSize, bool done);


/**
 * @brief      Automatically polls a bit of the command response.
 *
 * @param[in]  dev      The device ID. See table above.
 * @param[in]  cmd      The command.
 * @param[in]  timeout  The timeout. Must be 0-15. Tries = 31<<NspiClk + timeout.
 * @param[in]  off      The bit offset. Must be 0-7.
 * @param[in]  bitSet   Poll for a set ur unset bit.
 *
 * @return     Returns false on failure/timeout and true on success.
 */
#define NSPI_autoPollBit(dev, cmd, timeout, off, bitSet) _NSPI_autoPollBit(dev, (bitSet)<<30 | (off)<<24 | (timeout)<<16 | (cmd))
