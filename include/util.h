#pragma once

#include <ctype.h>
#include "types.h"

#define min(a,b)	(u32) a < (u32) b ? (u32) a : (u32) b

#define arrayEntries(array)	sizeof(array)/sizeof(*array)


void wait(u32 cycles);


// case insensitive string compare function
int strnicmp(const char *str1, const char *str2, u32 len);

// custom safe strncpy, string is always 0-terminated for buflen > 0
void strncpy_s(char *dest, const char *src, u32 nchars, const u32 buflen);


u32 getleu32(const void* ptr);

u32 swap32(u32 val);
