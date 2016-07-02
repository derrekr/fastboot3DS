#include "types.h"
#include "pxi.h"
//#include "interrupt.h"



void PXI_init(void)
{
	REG_PXI_SYNC11 = 0;
	REG_PXI_CNT11 = PXI_FLUSH_SEND_FIFO | PXI_ENABLE_SEND_RECV_FIFO;
}

void PXI_sendWord(u32 val)
{
	while(REG_PXI_CNT11 & PXI_SEND_FIFO_FULL);
	REG_PXI_SEND11 = val;
	REG_PXI_SYNC11 |= PXI_NOTIFY_9;
}

u32 PXI_recvWord(void)
{
	// Returns 0 for now if there is no data.
	// Will be fixed after implementing interrupts
	if(REG_PXI_CNT11 & PXI_RECV_FIFO_EMPTY) return 0;
	return REG_PXI_RECV11;
}
