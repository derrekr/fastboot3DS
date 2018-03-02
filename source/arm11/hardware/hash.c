/*
 *   This file is part of fastboot 3DS
 *   Copyright (C) 2018 derrek, profi200
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

#include <string.h>
#include "types.h"
#include "mem_map.h"
#include "arm11/hardware/hash.h"



//////////////////////////////////
//             HASH             //
//////////////////////////////////

#define HASH_REGS_BASE   (IO_MEM_ARM9_ARM11 + 0x1000)
#define REG_HASH_CNT     *((vu32*)(HASH_REGS_BASE + 0x00))
#define REG_HASH_BLKCNT  *((vu32*)(HASH_REGS_BASE + 0x04))
#define REG_HASH_HASH     ((u32* )(HASH_REGS_BASE + 0x40))
#define REG_HASH_INFIFO   (       (IO_MEM_ARM11_ONLY + 0x101000)) // INFIFO is in the DMA region


void HASH_start(u8 params)
{
	REG_HASH_CNT = params | HASH_ENABLE;
}

void HASH_update(const u32 *data, u32 size)
{
	while(size >= 64)
	{
		*((_u512*)REG_HASH_INFIFO) = *((const _u512*)data);
		data += 16;
		while(REG_HASH_CNT & HASH_ENABLE);

		size -= 64;
	}

	if(size) memcpy((void*)REG_HASH_INFIFO, data, size);
}

void HASH_finish(u32 *const hash, u8 endianess)
{
	REG_HASH_CNT = (REG_HASH_CNT & HASH_MODE_MASK) | endianess | HASH_FINAL_ROUND;
	while(REG_HASH_CNT & HASH_ENABLE);

	HASH_getState(hash);
}

void HASH_getState(u32 *const out)
{
	u32 stateSize;
	switch(REG_HASH_CNT & HASH_MODE_MASK)
	{
		case HASH_MODE_256:
			stateSize = 32;
			break;
		case HASH_MODE_224:
			stateSize = 28;
			break;
		case HASH_MODE_1:
			stateSize = 20;
			break;
		default:
			return;
	}

	memcpy(out, REG_HASH_HASH, stateSize);
}

void hash(const u32 *data, u32 size, u32 *const hash, u8 params, u8 hashEndianess)
{
	HASH_start(params);
	HASH_update(data, size);
	HASH_finish(hash, hashEndianess);
}
