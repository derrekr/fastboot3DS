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

#include "arm.h"
#include "types.h"


typedef enum
{
	IRQ_DMAC_1_0      = 0,  // DMAC_1 = NDMA
	IRQ_DMAC_1_1      = 1,
	IRQ_DMAC_1_2      = 2,
	IRQ_DMAC_1_3      = 3,
	IRQ_DMAC_1_4      = 4,
	IRQ_DMAC_1_5      = 5,
	IRQ_DMAC_1_6      = 6,
	IRQ_DMAC_1_7      = 7,
	IRQ_TIMER_0       = 8,
	IRQ_TIMER_1       = 9,
	IRQ_TIMER_2       = 10,
	IRQ_TIMER_3       = 11,
	IRQ_PXI_SYNC      = 12,
	IRQ_PXI_NOT_FULL  = 13,
	IRQ_PXI_NOT_EMPTY = 14,
	IRQ_AES           = 15,
	IRQ_SDIO_1        = 16,
	IRQ_SDIO_1_ASYNC  = 17,
	IRQ_SDIO_3        = 18,
	IRQ_SDIO_3_ASYNC  = 19,
	IRQ_DEBUG_RECV    = 20,
	IRQ_DEBUG_SEND    = 21,
	IRQ_RSA           = 22,
	IRQ_CTR_CARD_1    = 23, // SPICARD and CTRCARD too?
	IRQ_CTR_CARD_2    = 24,
	IRQ_CGC           = 25,
	IRQ_CGC_DET       = 26,
	IRQ_DS_CARD       = 27,
	IRQ_DMAC_2        = 28,
	IRQ_DMAC_2_ABORT  = 29
} Interrupt;


// IRQ handler type.
// id: contains the interrupt ID
typedef void (*IrqHandler)(u32 id);



/**
 * @brief      Initializes interrupts.
 */
void IRQ_init(void);

/**
 * @brief      Registers a interrupt handler and enables the specified interrupt.
 *
 * @param[in]  id       The interrupt ID. Must be <32.
 * @param[in]  handler  The interrupt handler to call.
 */
void IRQ_registerHandler(Interrupt id, IrqHandler handler);

/**
 * @brief      Unregisters the interrupt handler and disables the specified interrupt.
 *
 * @param[in]  id    The interrupt ID. Must be <32.
 */
void IRQ_unregisterHandler(Interrupt id);


static inline u32 enterCriticalSection(void)
{
	u32 tmp;
	__setCpsr_c((tmp = __getCpsr()) | PSR_I);
	return tmp & PSR_I;
}

static inline void leaveCriticalSection(u32 oldState)
{
	__setCpsr_c((__getCpsr() & ~PSR_I) | oldState);
}
