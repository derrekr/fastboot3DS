/**
 * 2016
 * profi200
 */

#include "types.h"
#include "cache.h"
#include "arm9/ndma.h"



void NDMA_copy(void *dest, const void *source, u32 num)
{
	flushDCacheRange(source, num<<2); // Make sure possible writes finished before DMAing.
	flushDCacheRange(dest, num<<2);

	REG_NDMA0_SRC_ADDR = (u32)source;
	REG_NDMA0_DST_ADDR = (u32)dest;
	REG_NDMA0_WRITE_CNT = num;
	REG_NDMA0_BLOCK_CNT = NDMA_BLOCK_SYS_FREQ;
	REG_NDMA0_CNT = NDMA_DST_UPDATE_INC | NDMA_SRC_UPDATE_INC | NDMA_IMMEDIATE_MODE | NDMA_ENABLE;

	while(REG_NDMA0_CNT & NDMA_ENABLE);

	invalidateDCacheRange(dest, num<<2); // Make sure no old data is in cache.
}

void NDMA_fill(void *dest, u32 value, u32 num)
{
	flushDCacheRange(dest, num<<2);

	REG_NDMA0_DST_ADDR = (u32)dest;
	REG_NDMA0_WRITE_CNT = num;
	REG_NDMA0_BLOCK_CNT = NDMA_BLOCK_SYS_FREQ;
	REG_NDMA0_FILL_DATA = value;
	REG_NDMA0_CNT = NDMA_DST_UPDATE_INC | NDMA_SRC_UPDATE_FILL | NDMA_IMMEDIATE_MODE | NDMA_ENABLE;

	while(REG_NDMA0_CNT & NDMA_ENABLE);

	invalidateDCacheRange(dest, num<<2);
}
