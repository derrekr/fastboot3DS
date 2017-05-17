#include "types.h"
#include "mem_map.h"
#include "pxi.h"



void NAKED firmLaunchStub(void)
{
	// Answer ARM0
	REG_PXI_SYNC11 = 0; // Disable all IRQs
	*((vu32*)0x1FFFFFFC) = 0;
	while(REG_PXI_CNT11 & PXI_SEND_FIFO_FULL);
	REG_PXI_SEND11 = PXI_RPL_OK;

	// Wait for entry address
	while(REG_PXI_CNT11 & PXI_RECV_FIFO_EMPTY);
	intptr_t e = (intptr_t)REG_PXI_RECV11;

	// Tell ARM9 we got the entry
	while(REG_PXI_CNT11 & PXI_SEND_FIFO_FULL);
	REG_PXI_SEND11 = PXI_RPL_FIRM_LAUNCH_READY;
	REG_PXI_CNT11 = 0; // Disable PXI

	if(!e)
	{
		while(!(e = (intptr_t)*((vu32*)0x1FFFFFFC)));
	}

	((void (*)(void))e)();
}

noreturn void firm_launch(void)
{
	// Relocate ARM11 stub
	for(u32 i = 0; i < A11_STUB_SIZE>>2; i++)
	{
		((u32*)A11_STUB_ENTRY)[i] = ((u32*)firmLaunchStub)[i];
	}

	((void (*)(void))A11_STUB_ENTRY)();
	while(1);
}
