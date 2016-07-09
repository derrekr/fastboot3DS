#pragma once

#include "types.h"

#define min(a,b)	(u32) a < (u32) b ? (u32) a : (u32) b

static void sleep_wait(u32 cycles)
{
	cycles >>= 2;
	while(cycles)
	{
		__asm("nop");
		cycles--;
	}
}

u32 getleu32(const void* ptr);
u32 swap32(u32 val);
