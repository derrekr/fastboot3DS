#pragma once

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

bool firm_size(size_t *size);
bool firm_verify(u32 fwSize, bool skipHashCheck, bool printInfo);
noreturn void firm_launch(void);
