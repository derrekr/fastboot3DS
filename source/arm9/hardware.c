#include "types.h"
#include "arm9/interrupt.h"
#include "arm9/ndma.h"
#include "arm9/timer.h"
#include "pxi.h"
#include "arm9/crypto.h"



void hardwareInit(void)
{
	IRQ_init();
	NDMA_init();
	TIMER_init();
	PXI_init();
	AES_init();
}

void hardwareDeinit(void)
{
	// New 3DS K9L doesn't like FIFO counts >4 and hangs.
	// Thx Nintendo
	AES_init();
}
