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
#include "mem_map.h"


#define FIRM_MAX_SIZE        (0x00400000)


typedef struct
{
	u32 offset;
	u32 address;
	u32 size;
	u32 copyMethod;
	u8 hash[0x20];
} firm_sectionheader;

typedef struct
{
	u32 magic;
	u32 priority;
	u32 entrypointarm11;
	u32 entrypointarm9;
	u8 reserved2[0x30];
	firm_sectionheader section[4];
	u8 signature[0x100];
} firm_header;



#ifdef __cplusplus
extern "C"
{
#endif

bool firm_size(size_t *size);
s32 loadVerifyFirm(const char *const path, bool skipHashCheck, bool installMode);
noreturn void firmLaunch(void);

#ifdef __cplusplus
}
#endif
