#pragma once

#include "types.h"


typedef enum
{
	IRQ_MPCORE_SW0  = 0,
	IRQ_MPCORE_SW1  = 1,
	IRQ_MPCORE_SW2  = 2,
	IRQ_MPCORE_SW3  = 3,
	IRQ_MPCORE_SW4  = 4,
	IRQ_MPCORE_SW5  = 5,
	IRQ_MPCORE_SW6  = 6,
	IRQ_MPCORE_SW7  = 7,
	IRQ_MPCORE_SW8  = 8,
	IRQ_MPCORE_SW9  = 9,
	IRQ_MPCORE_SW10 = 10,
	IRQ_MPCORE_SW11 = 11,
	IRQ_MPCORE_SW12 = 12,
	IRQ_MPCORE_SW13 = 13,
	IRQ_MPCORE_SW14 = 14,
	IRQ_MPCORE_SW15 = 15,
	IRQ_PXI_SYNC    = 80
} Interrupt;



void IRQ_init(void);
void IRQ_registerHandler(Interrupt id, u8 prio, u8 cpuMask, bool levHighActive, void (*irqHandler)(void));
//void IRQ_unregisterHandler(Interrupt id);

/**
 * @brief      Triggers a software interrupt for the specified CPUs.
 *
 * @param[in]  id       The interrupt ID. Must be <16.
 * @param[in]  cpuMask  The CPU mask. Each of the 4 bits stands for 1 core.
 */
void softwareInterrupt(Interrupt id, u8 cpuMask);

static inline void waitForIrq(void)
{
	__asm__ __volatile__("wfi" : :);
}

/*inline u32 enterCriticalSection(void)
{
}

inline void leaveCriticalSection(u32 oldState)
{
}*/
