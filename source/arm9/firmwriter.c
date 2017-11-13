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
