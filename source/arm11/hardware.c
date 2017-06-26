#include "types.h"
#include "arm11/interrupt.h"
#include "pxi.h"
#include "arm11/hid.h"



void hardwareInit(void)
{
	IRQ_init();
	PXI_init();
	hidInit();
}

/*void hardwareDeinit(void)
{
}*/
