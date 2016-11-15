#include "types.h"
#include "arm9/interrupt.h"
#include "arm9/ndma.h"
#include "arm9/timer.h"
#include "pxi.h"



void hardwareInit(void)
{
	// Atomic IRQ disable and aknowledge
	__asm__("stmia %0, {%1, %2}" : : "r" (&REG_IRQ_IE), "r" (0u), "r" (0xFFFFFFFFu) : "memory");
	NDMA_init();
	TIMER_init();
	PXI_init();
}
