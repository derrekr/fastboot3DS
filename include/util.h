#pragma once

#include "types.h"

#define min(a,b)	(u32) a < (u32) b ? (u32) a : (u32) b

static void wait(u32 cycles)
{
	cycles >>= 2;
	while(cycles)
	{
		__asm("nop");
		cycles--;
	}
}

// custom safe strncpy, string is always 0-terminated for buflen > 0
static void strncpy_s(char *dest, char *src, u32 nchars, u32 buflen)
{
	char c;

	if(!buflen)
		return;
		
	if(buflen > 1)
	{
		if(nchars >= buflen)
			nchars = buflen - 1;
		
		while(nchars--)
		{
			c = *src++;
			
			if(c == '\0')
				break;
			
			*dest++ = c;
		}
	}
	
	*dest = '\0';
}

u32 getleu32(const void* ptr);
u32 swap32(u32 val);
