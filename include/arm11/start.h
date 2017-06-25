#pragma once

#include "types.h"



void clearMem(u32 *adr, u32 size);
void deinitCpu(void);

static inline u32 getCpuId(void)
{
	u32 cpuId;
	__asm__ __volatile__("mrc p15, 0, %0, c0, c0, 5" : "=r" (cpuId) : );
	return cpuId & 3;
}
