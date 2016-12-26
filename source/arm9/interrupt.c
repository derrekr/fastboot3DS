#include "types.h"
#include "arm9/interrupt.h"


void (*irqHandlerTable[32])(void);



void IRQ_init(void)
{
	// Atomic IRQ disable and aknowledge
	__asm__ __volatile__("stmia %0, {%1, %2}" : : "r" (&REG_IRQ_IE), "r" (0u), "r" (0xFFFFFFFFu) : "memory");

	for(u32 i = 0; i < 32; i++)
	{
		irqHandlerTable[i] = (void (*)(void))NULL;
	}

	enableIrq();
}

void IRQ_registerHandler(Interrupt num, void (*irqHandler)(void))
{
	disableIrq();
	irqHandlerTable[num] = irqHandler;
	REG_IRQ_IE |= (1u<<num);
	enableIrq();
}

void IRQ_unregisterHandler(Interrupt num)
{
	disableIrq();
	REG_IRQ_IE &= ~(1u<<num);
	irqHandlerTable[num] = (void (*)(void))NULL;
	enableIrq();
}
