#pragma once

static void sleep_wait(u32 cycles)
{
	cycles >>= 2;
	while(cycles)
	{
		__asm("nop");
		cycles--;
	}
}
