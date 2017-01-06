#pragma once

#include "io.h"


#define waitForIrq()      __asm__ __volatile__("mcr p15, 0, %0, c7, c0, 4\n\t" : : "r" (0))


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
	IRQ_CTR_CARD_1    = 23,
	IRQ_CTR_CARD_2    = 24,
	IRQ_CGC           = 25,
	IRQ_CGC_DET       = 26,
	IRQ_DS_CARD       = 27,
	IRQ_DMAC_2        = 28,
	IRQ_DMAC_2_ABORT  = 29
} Interrupt;



void IRQ_init(void);
void IRQ_registerHandler(Interrupt num, void (*irqHandler)(void));
void IRQ_unregisterHandler(Interrupt num);

inline u32 enterCriticalSection(void)
{
	u32 tmp;
	__asm__ __volatile__("mrs %0, cpsr\n\t" : "=r" (tmp) : );
	__asm__ __volatile__("msr cpsr_c, %0\n\t" : : "r" (tmp | 0x80u));
	return tmp & 0x80u;
}

inline void leaveCriticalSection(u32 oldState)
{
	u32 tmp;
	__asm__ __volatile__("mrs %0, cpsr\n\t" : "=r" (tmp) : );
	__asm__ __volatile__("msr cpsr_c, %0\n\t" : : "r" ((tmp & ~(0x80u)) | oldState));
}
