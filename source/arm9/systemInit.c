#include "types.h"
#include "arm9/interrupt.h"
#include "pxi.h"



void initHardware(void)
{
	// Enable all interrupts but these
	REG_IRQ_IE = ~(u32)(IRQ_SDIO_1 | IRQ_SDIO_1_ASYNC | IRQ_SDIO_3 |
					IRQ_SDIO_3_ASYNC | IRQ_DEBUG_RECV | IRQ_DEBUG_SEND |
					IRQ_CTR_CARD_2 | IRQ_CGC | IRQ_CGC_DET |
					IRQ_DMAC_2 | IRQ_DMAC_2_ABORT);

	PXI_init();
}
