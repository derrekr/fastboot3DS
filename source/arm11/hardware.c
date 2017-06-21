#include "types.h"
#include "arm11/interrupt.h"
#include "pxi.h"



void hardwareInit(void)
{
	IRQ_init();
	PXI_init();
}

/*void hardwareDeinit(void)
{
}*/
