/**
 * 2016
 * profi200
 */

#include <assert.h>
#include "types.h"
#include "mem_map.h"
#include "arm9/ndma.h"
#include "arm9/interrupt.h"



void NDMA_init(void)
{
	REG_NDMA_GLOBAL_CNT = 0;

	for(u32 i = 0; i < 8; i++)
	{
		REG_NDMA_CNT(i) = 0;
	}

	REG_IRQ_IE |= (IRQ_DMAC_1_0 | IRQ_DMAC_1_1 | IRQ_DMAC_1_2 | IRQ_DMAC_1_3 |
					IRQ_DMAC_1_4 | IRQ_DMAC_1_5 | IRQ_DMAC_1_6 | IRQ_DMAC_1_7);
}

void NDMA_copy(u32 *dest, const u32 *source, u32 num)
{
	assert(((u32)dest >= ITCM_BOOT9_MIRROR + ITCM_SIZE) && (((u32)dest < DTCM_BASE) || ((u32)dest >= DTCM_BASE + DTCM_SIZE)));
	assert(((u32)source >= ITCM_BOOT9_MIRROR + ITCM_SIZE) && (((u32)source < DTCM_BASE) || ((u32)source >= DTCM_BASE + DTCM_SIZE)));

	REG_NDMA7_SRC_ADDR = (u32)source;
	REG_NDMA7_DST_ADDR = (u32)dest;
	REG_NDMA7_WRITE_CNT = num;
	REG_NDMA7_BLOCK_CNT = NDMA_BLOCK_SYS_FREQ;
	REG_NDMA7_CNT = NDMA_ENABLE | NDMA_IRQ_ENABLE | NDMA_IMMEDIATE_MODE | NDMA_SRC_UPDATE_INC | NDMA_DST_UPDATE_INC;

	while(!(REG_IRQ_IF & (u32)IRQ_DMAC_1_7))
	{
		waitForIrq();
	}
	REG_IRQ_IF = (u32)IRQ_DMAC_1_7;
}

void NDMA_fill(u32 *dest, u32 value, u32 num)
{
	assert(((u32)dest >= ITCM_BOOT9_MIRROR + ITCM_SIZE) && (((u32)dest < DTCM_BASE) || ((u32)dest >= DTCM_BASE + DTCM_SIZE)));

	REG_NDMA7_DST_ADDR = (u32)dest;
	REG_NDMA7_WRITE_CNT = num;
	REG_NDMA7_BLOCK_CNT = NDMA_BLOCK_SYS_FREQ;
	REG_NDMA7_FILL_DATA = value;
	REG_NDMA7_CNT = NDMA_ENABLE | NDMA_IRQ_ENABLE | NDMA_IMMEDIATE_MODE | NDMA_SRC_UPDATE_FILL | NDMA_DST_UPDATE_INC;

	while(!(REG_IRQ_IF & (u32)IRQ_DMAC_1_7))
	{
		waitForIrq();
	}
	REG_IRQ_IF = (u32)IRQ_DMAC_1_7;
}
