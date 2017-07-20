#pragma once

/*
 *   This file is part of fastboot 3DS
 *   Copyright (C) 2017 derrek, profi200
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "types.h"

#define min(a,b)	(u32) a < (u32) b ? (u32) a : (u32) b

#define arrayEntries(array)	sizeof(array)/sizeof(*array)


void wait(u32 cycles);


int mysscanf(const char *s, const char *fmt, ...);

// case insensitive string compare function
int strnicmp(const char *str1, const char *str2, u32 len);

// custom safe strncpy, string is always 0-terminated for buflen > 0
void strncpy_s(char *dest, const char *src, u32 nchars, const u32 buflen);

void memcpy_s(void *dstBuf, size_t dstBufSize, size_t dstBufOffset,
				void *srcBuf, size_t srcBufSize, size_t srcBufOffset, bool reverse);

u32 getleu32(const void* ptr);

u32 swap32(u32 val);
