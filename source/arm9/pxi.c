#include "types.h"
#include "pxi.h"
#include "arm9/interrupt.h"



void PXI_init(void)
{
	REG_PXI_SYNC9 = PXI_IRQ_ENABLE;
	REG_PXI_CNT9 = PXI_FLUSH_SEND_FIFO | PXI_ENABLE_SEND_RECV_FIFO;
}

void PXI_sendWord(u32 val)
{
	while(REG_PXI_CNT9 & PXI_SEND_FIFO_FULL);
	REG_PXI_SEND9 = val;
	REG_PXI_SYNC9 |= PXI_NOTIFY_11;
}

bool PXI_trySendWord(u32 val)
{
	if(REG_PXI_CNT9 & PXI_SEND_FIFO_FULL)
		return false;
	REG_PXI_SEND9 = val;
	REG_PXI_SYNC9 |= PXI_NOTIFY_11;
	return true;
}

u32 PXI_recvWord(void)
{
	while(REG_PXI_CNT9 & PXI_RECV_FIFO_EMPTY);
	return REG_PXI_RECV9;
}

u32 PXI_tryRecvWord(bool *success)
{
	if(REG_PXI_CNT9 & PXI_RECV_FIFO_EMPTY);
	{
		*success = false;
		return 0;
	}

	*success = true;
	return REG_PXI_RECV9;
}
