#pragma once

#include "io.h"



typedef enum
{
	INTERRUPT_DMAC_1_0      = (1u<<0), // DMAC_1 = NDMA
	INTERRUPT_DMAC_1_1      = (1u<<1),
	INTERRUPT_DMAC_1_2      = (1u<<2),
	INTERRUPT_DMAC_1_3      = (1u<<3),
	INTERRUPT_DMAC_1_4      = (1u<<4),
	INTERRUPT_DMAC_1_5      = (1u<<5),
	INTERRUPT_DMAC_1_6      = (1u<<6),
	INTERRUPT_DMAC_1_7      = (1u<<7),
	INTERRUPT_TIMER_0       = (1u<<8),
	INTERRUPT_TIMER_1       = (1u<<9),
	INTERRUPT_TIMER_2       = (1u<<10),
	INTERRUPT_TIMER_3       = (1u<<11),
	INTERRUPT_PXI_SYNC      = (1u<<12),
	INTERRUPT_PXI_NOT_FULL  = (1u<<13),
	INTERRUPT_PXI_NOT_EMPTY = (1u<<14),
	INTERRUPT_AES           = (1u<<15),
	INTERRUPT_SDIO_1        = (1u<<16),
	INTERRUPT_SDIO_1_ASYNC  = (1u<<17),
	INTERRUPT_SDIO_3        = (1u<<18),
	INTERRUPT_SDIO_3_ASYNC  = (1u<<19),
	INTERRUPT_DEBUG_RECV    = (1u<<20),
	INTERRUPT_DEBUG_SEND    = (1u<<21),
	INTERRUPT_RSA           = (1u<<22),
	INTERRUPT_CTR_CARD_1    = (1u<<23),
	INTERRUPT_CTR_CARD_2    = (1u<<24),
	INTERRUPT_CGC           = (1u<<25),
	INTERRUPT_CGC_DET       = (1u<<26),
	INTERRUPT_DS_CARD       = (1u<<27),
	INTERRUPT_DMAC_2        = (1u<<28),
	INTERRUPT_DMAC_2_ABORT  = (1u<<29)
} Interrupt;
