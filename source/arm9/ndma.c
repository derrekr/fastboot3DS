/**
 * 2017
 * profi200
 */

#include <assert.h>
#include "types.h"
#include "arm9/ndma.h"
#include "arm9/interrupt.h"



void NDMA_init(void)
{
	for(u32 i = 0; i < 8; i++)
	{
		REG_NDMA_CNT(i) = (REG_NDMA_CNT(i)<<1)>>1;
	}

	REG_NDMA_GLOBAL_CNT = NDMA_ROUND_ROBIN(32);

	IRQ_registerHandler(IRQ_DMAC_1_7, NULL);
}

void NDMA_copyAsync(u32 *dest, const u32 *source, u32 size)
{
	assert(((u32)dest >= ITCM_BOOT9_MIRROR + ITCM_SIZE) && (((u32)dest < DTCM_BASE) || ((u32)dest >= DTCM_BASE + DTCM_SIZE)));
	assert(((u32)source >= ITCM_BOOT9_MIRROR + ITCM_SIZE) && (((u32)source < DTCM_BASE) || ((u32)source >= DTCM_BASE + DTCM_SIZE)));

	REG_NDMA7_SRC_ADDR = (u32)source;
	REG_NDMA7_DST_ADDR = (u32)dest;
	REG_NDMA7_LOG_BLK_CNT = size / 4;
	REG_NDMA7_INT_CNT = NDMA_INT_SYS_FREQ;
	REG_NDMA7_CNT = NDMA_ENABLE | NDMA_IRQ_ENABLE | NDMA_STARTUP_IMMEDIATE | NDMA_SRC_UPDATE_INC | NDMA_DST_UPDATE_INC;
}

void NDMA_copy(u32 *dest, const u32 *source, u32 size)
{
	NDMA_copyAsync(dest, source, size);

	while(REG_NDMA7_CNT & NDMA_ENABLE)
	{
		waitForInterrupt();
	}
}

void NDMA_fillAsync(u32 *dest, u32 value, u32 size)
{
	assert(((u32)dest >= ITCM_BOOT9_MIRROR + ITCM_SIZE) && (((u32)dest < DTCM_BASE) || ((u32)dest >= DTCM_BASE + DTCM_SIZE)));

	REG_NDMA7_DST_ADDR = (u32)dest;
	REG_NDMA7_LOG_BLK_CNT = size / 4;
	REG_NDMA7_INT_CNT = NDMA_INT_SYS_FREQ;
	REG_NDMA7_FILL_DATA = value;
	REG_NDMA7_CNT = NDMA_ENABLE | NDMA_IRQ_ENABLE | NDMA_STARTUP_IMMEDIATE | NDMA_SRC_UPDATE_FILL | NDMA_DST_UPDATE_INC;
}

void NDMA_fill(u32 *dest, u32 value, u32 size)
{
	NDMA_fillAsync(dest, value, size);

	while(REG_NDMA7_CNT & NDMA_ENABLE)
	{
		waitForInterrupt();
	}
}
