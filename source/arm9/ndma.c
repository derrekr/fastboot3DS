/**
 * 2016
 * profi200
 */

#include "types.h"
#include "arm9/ndma.h"
#include "arm9/interrupt.h"



void NDMA_copy(u32 *dest, const u32 *source, u32 num)
{
	REG_IRQ_IE |= (u32)INTERRUPT_DMAC_1_7;

	REG_NDMA7_SRC_ADDR = (u32)source;
	REG_NDMA7_DST_ADDR = (u32)dest;
	REG_NDMA7_WRITE_CNT = num;
	REG_NDMA7_BLOCK_CNT = NDMA_BLOCK_SYS_FREQ;
	REG_NDMA7_CNT = NDMA_ENABLE | NDMA_INTERRUPT_ENABLE | NDMA_IMMEDIATE_MODE | NDMA_SRC_UPDATE_INC | NDMA_DST_UPDATE_INC;

	while(!(REG_IRQ_IF & (u32)INTERRUPT_DMAC_1_7))
	{
		__asm__("mcr p15, 0, %[in], c7, c0, 4\n" : : [in] "r" (0));
	}
	REG_IRQ_IE &= ~((u32)INTERRUPT_DMAC_1_7);
	REG_IRQ_IF = (u32)INTERRUPT_DMAC_1_7;
}

void NDMA_fill(u32 *dest, u32 value, u32 num)
{
	REG_IRQ_IE |= (u32)INTERRUPT_DMAC_1_7;

	REG_NDMA7_DST_ADDR = (u32)dest;
	REG_NDMA7_WRITE_CNT = num;
	REG_NDMA7_BLOCK_CNT = NDMA_BLOCK_SYS_FREQ;
	REG_NDMA7_FILL_DATA = value;
	REG_NDMA7_CNT = NDMA_ENABLE | NDMA_INTERRUPT_ENABLE | NDMA_IMMEDIATE_MODE | NDMA_SRC_UPDATE_FILL | NDMA_DST_UPDATE_INC;

	while(!(REG_IRQ_IF & (u32)INTERRUPT_DMAC_1_7))
	{
		__asm__("mcr p15, 0, %[in], c7, c0, 4\n" : : [in] "r" (0));
	}
	REG_IRQ_IE &= ~((u32)INTERRUPT_DMAC_1_7);
	REG_IRQ_IF = (u32)INTERRUPT_DMAC_1_7;
}
