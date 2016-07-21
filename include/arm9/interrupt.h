#pragma once

#include "io.h"



#define waitForIrq()      __asm__("mcr p15, 0, %[in], c7, c0, 4\n" : : [in] "r" (0))


typedef enum
{
	IRQ_DMAC_1_0      = (1<<0 ), // DMAC_1 = NDMA
	IRQ_DMAC_1_1      = (1<<1 ),
	IRQ_DMAC_1_2      = (1<<2 ),
	IRQ_DMAC_1_3      = (1<<3 ),
	IRQ_DMAC_1_4      = (1<<4 ),
	IRQ_DMAC_1_5      = (1<<5 ),
	IRQ_DMAC_1_6      = (1<<6 ),
	IRQ_DMAC_1_7      = (1<<7 ),
	IRQ_TIMER_0       = (1<<8 ),
	IRQ_TIMER_1       = (1<<9 ),
	IRQ_TIMER_2       = (1<<10),
	IRQ_TIMER_3       = (1<<11),
	IRQ_PXI_SYNC      = (1<<12),
	IRQ_PXI_NOT_FULL  = (1<<13),
	IRQ_PXI_NOT_EMPTY = (1<<14),
	IRQ_AES           = (1<<15),
	IRQ_SDIO_1        = (1<<16),
	IRQ_SDIO_1_ASYNC  = (1<<17),
	IRQ_SDIO_3        = (1<<18),
	IRQ_SDIO_3_ASYNC  = (1<<19),
	IRQ_DEBUG_RECV    = (1<<20),
	IRQ_DEBUG_SEND    = (1<<21),
	IRQ_RSA           = (1<<22),
	IRQ_CTR_CARD_1    = (1<<23),
	IRQ_CTR_CARD_2    = (1<<24),
	IRQ_CGC           = (1<<25),
	IRQ_CGC_DET       = (1<<26),
	IRQ_DS_CARD       = (1<<27),
	IRQ_DMAC_2        = (1<<28),
	IRQ_DMAC_2_ABORT  = (1<<29)
} Interrupt;
