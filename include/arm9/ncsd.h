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
