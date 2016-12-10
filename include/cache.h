#pragma once

#include "types.h"



void invalidateICache(void);
void invalidateICacheRange(void *base, u32 size);
void flushDCache(void);
void flushDCacheRange(void *base, u32 size);
void invalidateDCache(void);
void invalidateDCacheRange(void *base, u32 size);
