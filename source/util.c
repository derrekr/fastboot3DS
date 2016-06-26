#include "types.h"

u32 getleu32(const void* ptr)
{
	const u8* p = (const u8*)ptr;
	
	return (u32)((p[0]<<0) | (p[1]<<8) | (p[2]<<16) | (p[3]<<24));
}

u32 swap32(u32 val)
{
	return ((val>>24)|
			(val>>8  & 0x0000FF00)|
			(val<<8  & 0x00FF0000)|
			(val<<24));
}
