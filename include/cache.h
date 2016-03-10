#pragma once

#include "types.h"



void invalidateICache(void);
void flushDCache(void);
#ifdef ARM9
void invalidateICacheRange(const void *base, u32 size);
void flushDCacheRange(const void *base, u32 size);
void invalidateDCache(void);
void invalidateDCacheRange(const void *base, u32 size);
#endif
