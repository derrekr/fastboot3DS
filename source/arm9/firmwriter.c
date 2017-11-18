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

#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "firmwriter.h"
#include "mem_map.h"
#include "arm9/partitions.h"
#include "arm9/firm.h"
#include "arm9/dev.h"
#include "util.h"
#include "arm9/hardware/crypto.h"
#include "fastboot3DS_pubkey_bin.h"



s32 writeFirmPartition(const char *const part)
{
	if(memcmp(part, "firm", 4) != 0) return -1;
	if(!dev_decnand->is_active()) return -2;

	size_t partInd, sector;
	if(!partitionGetIndex(part, &partInd)) return -3;
	if(!partitionGetSectorOffset(partInd, &sector)) return -4;

	size_t firmSize;
	if(!firm_size(&firmSize)) return -5;

	u8 *firmBuf = (u8*)FIRM_LOAD_ADDR;
	u8 *const cmpBuf = (u8*)malloc(FIRMWRITER_BLK_SIZE);
	if(!cmpBuf) return -6;

	while(firmSize)
	{
		const u32 writeSize = min(firmSize, FIRMWRITER_BLK_SIZE);

		if(!dev_decnand->write_sector(sector, writeSize>>9, firmBuf)) return -7;
		if(!dev_decnand->read_sector(sector, writeSize>>9, cmpBuf)) return -7;
		if(memcmp(firmBuf, cmpBuf, writeSize) != 0)
		{
			free(cmpBuf);
			return -8;
		}

		sector += writeSize>>9;
		firmSize -= writeSize;
		firmBuf += writeSize;
	}

	free(cmpBuf);

	return 0;
}

s32 loadVerifyUpdate(const char *const path, u32 *const version)
{
	if(!dev_decnand->is_active()) return -1;
	if(loadVerifyFirm(path, false, true) < 0) return UPDATE_ERR_INVALID_FIRM;

	u32 *updateBuffer = (u32*)FIRM_LOAD_ADDR;
#ifdef NDEBUG
	// Verify signature
	if(!RSA_setKey2048(3, fastboot3DS_pubkey_bin, 0x01000100) ||
	   !RSA_verify2048(updateBuffer + 0x40, updateBuffer, 0x100))
		return UPDATE_ERR_INVALID_SIG;
#endif

	// verify fastboot magic
	if(memcmp((void*)updateBuffer + 0x200, "FASTBOOT 3DS   ", 16) != 0)
		return -4;

	// Check version
	const u32 vers = *(u32*)((void*)updateBuffer + 0x210);
	if(vers < ((u32)VERS_MAJOR<<16 | VERS_MINOR)) return UPDATE_ERR_DOWNGRADE;

	size_t partInd, sector;
	if(!partitionGetIndex("firm0", &partInd)) return -6;
	if(!partitionGetSectorOffset(partInd, &sector)) return -7;

	u8 *firm0Buf = (u8*)malloc(0x200);
	if(!firm0Buf) return -8;
	if(!dev_decnand->read_sector(sector + 1, 1, firm0Buf))
	{
		free(firm0Buf);
		return -9;
	}

	// verify fastboot is installed to firm0:/
	if(memcmp(firm0Buf, "FASTBOOT 3DS   ", 16) != 0)
	{
		free(firm0Buf);
		return UPDATE_ERR_NOT_INSTALLED;
	}

	free(firm0Buf);
	if(version) *version = vers;

	return 0;
}
