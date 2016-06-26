#pragma once

#include "types.h"



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
