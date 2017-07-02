#include "types.h"
#include "arm11/start.h"
#include "arm11/interrupt.h"
#include "arm11/timer.h"
#include "pxi.h"
#include "arm11/hid.h"



void hardwareInit(void)
{
	IRQ_init();
	TIMER_init();

	if(!getCpuId())
	{
		PXI_init();
		hidInit();
	}
	else
	{
		// We don't need core 1 yet so back it goes into boot11.
		deinitCpu();
		((void (*)(void))0x0001004C)();
	}
}

/*void hardwareDeinit(void)
{
}*/
