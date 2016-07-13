#include "types.h"
#include "pxi.h"
#include "arm9/interrupt.h"



void PXI_init(void)
{
	REG_IRQ_IE |= INTERRUPT_PXI_SYNC;
	REG_PXI_SYNC9 = PXI_INTERRUPT_ENABLE;
	REG_PXI_CNT9 = PXI_FLUSH_SEND_FIFO | PXI_ENABLE_SEND_RECV_FIFO;
}

void PXI_sendWord(u32 val)
{
	while(REG_PXI_CNT9 & PXI_SEND_FIFO_FULL);
	REG_PXI_SEND9 = val;
	REG_PXI_SYNC9 |= PXI_NOTIFY_11;
}

u32 PXI_recvWord(void)
{
	while(REG_PXI_CNT9 & PXI_RECV_FIFO_EMPTY);
	return REG_PXI_RECV9;
}
