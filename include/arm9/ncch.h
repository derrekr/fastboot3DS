#pragma once

#include "types.h"



typedef struct
{
	union
	{
		u8 signature[0x100];
		struct
		{
			u32 seedKeyY[4];
			u32 encryptedSeed[4];
			u32 seedAesMac[4];
			u32 seedNonce[3];
			u8 padding[0xC4];
		};
	};
	u32 magic;
	u32 contentMediaSize;
	u64 partId;
	char makerCode[2];
	u16 version;
	u32 seedHashTestVector;
	u64 programId;
	u8 reserved[0x10];
	u8 logoRegionHash[0x20];
	char productCode[0x10];
	u8 exheaderHash[0x20];
	u32 exheaderSize;
	u8 reserved2[4];
	u8 flags[8];
	u32 plainRegionMediaOffset;
	u32 plainRegionMediaSize;
	u32 logoRegionMediaOffset;
	u32 logoRegionMediaSize;
	u32 exeFsMediaOffset;
	u32 exeFsMediaSize;
	u32 exeFsHashRegionMediaSize;
	u8 reserved3[4];
	u32 romFsMediaOffset;
	u32 romFsMediaSize;
	u32 romFsHashRegionMediaSize;
	u8 reserved4[4];
	u8 exeFsSuperblockHash[0x20];
	u8 romFsSuperblockHash[0x20];
} NCCH_header;
