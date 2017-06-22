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



/**
 * @brief      Initializes the generic interrupt controller.
 */
void IRQ_init(void);

/**
 * @brief      Registers a interrupt handler and enables the specified interrupt.
 *
 * @param[in]  id             The interrupt ID. Must be <128.
 * @param[in]  prio           The priority. 0 = highest, 14 = lowest, 15 = disabled
 * @param[in]  cpuMask        The CPU mask. Each of the 4 bits stands for 1 core. 0 means current CPU.
 * @param[in]  levHighActive  Set to true to make the interrupt level high active.
 * @param[in]  irqHandler     The interrupt handler to call.
 */
void IRQ_registerHandler(Interrupt id, u8 prio, u8 cpuMask, bool levHighActive, void (*irqHandler)(void));

/**
 * @brief      Unregisters the interrupt handler and disables the specified interrupt.
 *
 * @param[in]  id    The interrupt ID. Must be <128.
 */
void IRQ_unregisterHandler(Interrupt id);

/**
 * @brief      Sets the priority of an interrupt.
 *
 * @param[in]  id    The interrupt ID. Must be <128.
 * @param[in]  prio  The priority. 0 = highest, 14 = lowest, 15 = disabled
 */
void IRQ_setPriority(Interrupt id, u8 prio);

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
