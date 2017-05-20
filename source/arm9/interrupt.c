#include "types.h"
#include "arm9/interrupt.h"


void (*irqHandlerTable[32])(void);



void IRQ_init(void)
{
	REG_IRQ_IE = 0;
	REG_IRQ_IF = 0xFFFFFFFFu;

	for(u32 i = 0; i < 32; i++)
	{
		irqHandlerTable[i] = (void (*)(void))NULL;
	}

	leaveCriticalSection(0u); // Abuse it to enable IRQ
}

void IRQ_registerHandler(Interrupt num, void (*irqHandler)(void))
{
	const u32 oldState = enterCriticalSection();

	irqHandlerTable[num] = irqHandler;
	REG_IRQ_IE |= 1u<<num;

	leaveCriticalSection(oldState);
}

void IRQ_unregisterHandler(Interrupt num)
{
	const u32 oldState = enterCriticalSection();

	REG_IRQ_IE &= ~(1u<<num);
	irqHandlerTable[num] = (void (*)(void))NULL;

	leaveCriticalSection(oldState);
}
