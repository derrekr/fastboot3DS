#pragma once

#include "types.h"



void invalidateICache(void);
void invalidateICacheRange(const void *const base, u32 size);
void flushDCache(void);
void flushInvalidateDCache(void);
void flushDCacheRange(const void *const base, u32 size);
void flushInvalidateDCacheRange(const void *const base, u32 size);
void invalidateDCache(void);
void invalidateDCacheRange(const void *const base, u32 size);
