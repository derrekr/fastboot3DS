#include "types.h"
#include "mem_map.h"
#include "arm9/interrupt.h"


#define IRQ_REGS_BASE  (IO_MEM_ARM9_ONLY + 0x1000)
#define REG_IRQ_IE     *((vu32*)(IRQ_REGS_BASE + 0x00))
#define REG_IRQ_IF     *((vu32*)(IRQ_REGS_BASE + 0x04))


IrqHandler irqHandlerTable[32] = {0};



void IRQ_init(void)
{
	REG_IRQ_IE = 0;
	REG_IRQ_IF = 0xFFFFFFFFu;

	leaveCriticalSection(0u); // Abuse it to enable IRQ
}

void IRQ_registerHandler(Interrupt id, IrqHandler handler)
{
	const u32 oldState = enterCriticalSection();

	irqHandlerTable[id] = handler;
	REG_IRQ_IE |= 1u<<id;

	leaveCriticalSection(oldState);
}

void IRQ_unregisterHandler(Interrupt id)
{
	const u32 oldState = enterCriticalSection();

	REG_IRQ_IE &= ~(1u<<id);
	irqHandlerTable[id] = (IrqHandler)NULL;

	leaveCriticalSection(oldState);
}
