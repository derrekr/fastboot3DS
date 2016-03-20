#pragma once

#include "types.h"



// Stolen from ctrtool and adjusted
typedef struct
{
	u32 offset;
	u32 size;
} Ncsd_part;

typedef struct
{
	u8  signature[0x100];
	u32 magic;
	u32 mediaSize;
	u64 mediaId;
	u8  partFsType[8];
	u8  partCryptType[8];
	Ncsd_part part[8];
	u8  unused[0xA0];
} Ncsd_header;
