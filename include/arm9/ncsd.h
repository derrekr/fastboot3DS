#pragma once

#include "types.h"



typedef struct
{
	u8 signature[0x100];
	u32 magic;
	u32 mediaSize;
	u64 mediaId;
	u8 partFsType[8];
	u8 partCryptType[8];
	struct NCSD_part
	{
		u32 mediaOffset;
		u32 mediaSize;
	} partitions[8];
	u8 exheaderHash[0x20];
	u32 additionalHeaderSize;
	u32 sector0Offset;
	u8 partFlags[8];
	u64 partIdTable[8];
	u8 reserved[0x20];
	u8 reserved2[0xE];
	u8 apCheckBits;
	u8 saveCrypt96Flag;
} NCSD_header;
