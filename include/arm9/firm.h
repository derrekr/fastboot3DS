#pragma once

#include "types.h"
#include "mem_map.h"

#define FIRM_LOAD_ADDR  (VRAM_BASE + 0x200000)
#define FIRM_MAX_SIZE   (0x00400000)



typedef struct
{
	u32 offset;
	u32 address;
	u32 size;
	u32 type;
	u8 hash[0x20];
} firm_sectionheader;

typedef struct
{
	u8 magic[4];
	u8 reserved1[4];
	u32 entrypointarm11;
	u32 entrypointarm9;
	u8 reserved2[0x30];
	firm_sectionheader section[4];
	u8 signature[0x100];
} firm_header;

// TODO
/*
// <first word from signaure>, <version #>
const u32 firmVersionTable[] = {
	0, 0
};

// <version #><
const u32 firmVersionStringTable[] = {
	0, 0
};
*/


bool firm_load_verify(u32 firmSize);
noreturn void firm_launch(void);
