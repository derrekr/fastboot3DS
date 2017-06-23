#include "types.h"
#include "mem_map.h"


#define REGs_GPIO   ((vu16*)(IO_MEM_ARM9_ARM11 + 0x47000))



void gpioSetBit(u16 reg, u8 bitNum)
{
	REGs_GPIO[reg] |= 1u<<bitNum;
}

void gpioClearBit(u16 reg, u8 bitNum)
{
	REGs_GPIO[reg] &= ~(1u<<bitNum);
}
