#include "types.h"
#include "arm9/interrupt.h"
#include "pxi.h"



void initHardware(void)
{
	// Enable all interrupts but these
	REG_IRQ_IE = ~(u32)(INTERRUPT_SDIO_1 | INTERRUPT_SDIO_1_ASYNC | INTERRUPT_SDIO_3 |
					INTERRUPT_SDIO_3_ASYNC | INTERRUPT_DEBUG_RECV | INTERRUPT_DEBUG_SEND |
					INTERRUPT_CTR_CARD_2 | INTERRUPT_CGC | INTERRUPT_CGC_DET |
					INTERRUPT_DMAC_2 | INTERRUPT_DMAC_2_ABORT);

	PXI_init();
}
