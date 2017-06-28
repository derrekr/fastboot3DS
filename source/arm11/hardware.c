#include "types.h"
#include "arm11/interrupt.h"
#include "arm11/timer.h"
#include "pxi.h"
#include "arm11/hid.h"



void hardwareInit(void)
{
	IRQ_init();
	TIMER_init();
	PXI_init();
	hidInit();
}

/*void hardwareDeinit(void)
{
}*/
