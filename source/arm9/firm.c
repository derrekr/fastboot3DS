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

#include <string.h>
#include "types.h"
// we need the arm11 mem map information
#define ARM11
#include "mem_map.h"
#undef ARM11
#include "arm9/firm.h"
#include "arm9/start.h"
#include "util.h"
#include "arm9/hardware/crypto.h"
#include "arm9/hardware/ndma.h"
#include "hardware/pxi.h"
#include "arm9/partitions.h"
#include "arm9/dev.h"
#include "fs.h"
#include "hardware/gfx.h"
#include "system.h"


typedef struct
{
	u32 addr;
	u32 size;
} FirmWhitelist;


static const FirmWhitelist bootWhitelist[] =
{
	{ // Unused ITCM data
		ITCM_KERNEL_MIRROR, ITCM_SIZE - 0x4800
	},
	{ // ARM9 memory excluding exception vectors
		A9_RAM_BASE + 0x40, A9_RAM_SIZE + A9_RAM_N3DS_EXT_SIZE - 0x40
	},
	{ // VRAM excluding the FIRM buffer
		VRAM_BASE, VRAM_SIZE - 0x400000
	},
	{ // DSP memory + AXIWRAM excluding stack and FIRM launch stub
		DSP_MEM_BASE, DSP_MEM_SIZE + AXIWRAM_SIZE - 0x220
	},
	{ // FCRAM
		FCRAM_BASE, FCRAM_SIZE + FCRAM_N3DS_EXT_SIZE
	}
};

static const FirmWhitelist installWhitelist[] =
{
	{ // ITCM
		ITCM_KERNEL_MIRROR, ITCM_SIZE + ITCM_SIZE // Kernel and boot9 mirror
	},
	{ // ARM9 memory
		A9_RAM_BASE, A9_RAM_SIZE + A9_RAM_N3DS_EXT_SIZE
	},
	{ // IO mem
		IO_MEM_BASE, 0x400000
	},
	{ // VRAM
		VRAM_BASE, VRAM_SIZE
	},
	{ // DSP memory + AXIWRAM
		DSP_MEM_BASE, DSP_MEM_SIZE + AXIWRAM_SIZE
	}
};

static int firmLaunchArgc;



/* Calculates the actual firm partition size by using its header */
bool firm_size(size_t *size, const firm_header *const hdr)
{
	u32 curLen = sizeof(firm_header);
	u32 curOffset = 0;
	*size = 0;
	
	/* scan sections in reverse order */
	for(int i=3; i>=0; i--)
	{
		const firm_sectionheader *const section = &hdr->section[i];

		if(section->size == 0)
			continue;
		
		if(section->offset <= curOffset)
			continue;
		
		curOffset = section->offset;
			
		if(section->size > FIRM_MAX_SIZE || curOffset >= FIRM_MAX_SIZE)
			return false;
		
		if(curLen < curOffset + section->size)
			curLen = curOffset + section->size;
		
		if(curLen > FIRM_MAX_SIZE)
			return false;
	}
	
	*size = curLen;
	
	return true;
}

// NOTE: Do not call any functions here!
void NAKED firmLaunchStub(int argc, const char **argv)
{	
	firm_header *firm_hdr = (firm_header*)FIRM_LOAD_ADDR;
	void (*entry9)(int, const char**, u32) = (void (*)(int, const char**, u32))firm_hdr->entrypointarm9;
	u32 entry11 = firm_hdr->entrypointarm11;


	REG_PXI_SYNC = 0; // Disable all IRQs
	while(1)
	{
		// Wait for the ARM11 to be ready before copying sections
		while(REG_PXI_CNT & PXI_RECV_FIFO_EMPTY);
		if(REG_PXI_RECV == 0xA8E4u) break;
	}

	for(u32 i = 0; i < 4; i++)
	{
		firm_sectionheader *section = &firm_hdr->section[i];
		if(section->size == 0)
			continue;

		// Use NDMA for everything but copy method 2
		if(section->copyMethod < 2)
		{
			REG_NDMA_SRC_ADDR(i) = FIRM_LOAD_ADDR + section->offset;
			REG_NDMA_DST_ADDR(i) = section->address;
			REG_NDMA_LOG_BLK_CNT(i) = section->size / 4;
			REG_NDMA_INT_CNT(i) = NDMA_INT_SYS_FREQ;
			REG_NDMA_CNT(i) = NDMA_ENABLE | NDMA_BURST_SIZE(128) | NDMA_IMMEDIATE_MODE |
			                  NDMA_SRC_UPDATE_INC | NDMA_DST_UPDATE_INC;
		}
		else
		{
			u32 *dst = (u32*)section->address;
			u32 *src = (u32*)(FIRM_LOAD_ADDR + section->offset);

			for(u32 n = 0; n < section->size / 4; n += 4)
			{
				dst[n + 0] = src[n + 0];
				dst[n + 1] = src[n + 1];
				dst[n + 2] = src[n + 2];
				dst[n + 3] = src[n + 3];
			}
		}
	}

	while(REG_NDMA0_CNT & NDMA_ENABLE || REG_NDMA1_CNT & NDMA_ENABLE ||
	      REG_NDMA2_CNT & NDMA_ENABLE || REG_NDMA3_CNT & NDMA_ENABLE);

	// Tell ARM11 its entrypoint
	REG_PXI_SEND = entry11;

	while(1)
	{
		// Wait for the ARM111 to confirm it received the entrypoint
		while(REG_PXI_CNT & PXI_RECV_FIFO_EMPTY);
		if(REG_PXI_RECV == 0x94C6u) break;
	}

	REG_PXI_CNT = 0; // Disable PXI

	// go for it!
	entry9(argc, argv, 0x3BEEFu);
}

s32 loadVerifyFirm(const char *const path, bool skipHashCheck, bool installMode)
{
	u32 firmSize;
	firm_header *const firmHdr = (firm_header*)FIRM_LOAD_ADDR;


	if(memcmp(path, "firm", 4) == 0)
	{
		if(!dev_decnand->is_active()) return -1;

		size_t partInd, sector;
		if(!partitionGetIndex(path, &partInd)) return -2;
		if(!partitionGetSectorOffset(partInd, &sector)) return -3;

		if(!dev_decnand->read_sector(sector, 1, (void*)FIRM_LOAD_ADDR)) return -4;
		if(!firm_size((size_t*)&firmSize, firmHdr)) return -5;
		sector++;
		if(!dev_decnand->read_sector(sector, (firmSize>>9) - 1, (void*)(FIRM_LOAD_ADDR + sizeof(firm_header))))
			return -4;
	}
	else if(memcmp(path, "ram", 3) == 0)
	{
		firm_header *const ramBootHdr = (firm_header*)RAM_FIRM_BOOT_ADDR;
		if(memcmp(&ramBootHdr->magic, "FIRM", 4) == 0)
		{
			if(!firm_size((size_t*)&firmSize, ramBootHdr)) return -5;
			NDMA_copy((u32*)FIRM_LOAD_ADDR, (u32*)RAM_FIRM_BOOT_ADDR, firmSize);
			ramBootHdr->magic = 0;
		}
		else return -6;
	}
	else
	{
		const s32 f = fOpen(path, FS_OPEN_EXISTING | FS_OPEN_READ);
		if(f < 0) return -6;

		firmSize = fSize(f);
		if(firmSize > FIRM_MAX_SIZE)
		{
			fClose(f);
			return -7;
		}
		if(fRead(f, (void*)FIRM_LOAD_ADDR, firmSize) < 0)
		{
			fClose(f);
			return -8;
		}

		fClose(f);
	}


	// Check if <= FIRM header size
	if(firmSize <= sizeof(firm_header)) return -9;

	// Check magic
	if(memcmp(&firmHdr->magic, "FIRM", 4) != 0) return -10;

	// ARM9 entrypoint must not be 0
	if(firmHdr->entrypointarm9 == 0) return -11;

	for(u32 i = 0; i < 4; i++)
	{
		const firm_sectionheader *const section = &firmHdr->section[i];
		const u32 secSize = section->size;

		if(!secSize) continue;

		const u32 secOffset = section->offset;
		// Check section offset
		if(secOffset >= firmSize || secOffset < sizeof(firm_header)) return -12;

		// Check section size
		if(secSize >= firmSize || (secSize + secOffset > firmSize)) return -13;

		const FirmWhitelist *list;
		u32 listSize;
		if(installMode)
		{
			list = installWhitelist;
			listSize = arrayEntries(installWhitelist);
		}
		else
		{
			list = bootWhitelist;
			listSize = arrayEntries(bootWhitelist);
		}
		const u32 secAddr = section->address;
		bool allowed = false;
		for(u32 n = 0; n < listSize; n++)
		{
			const u32 addr = list[n].addr;
			const u32 size = list[n].size;

			// Overflow check
			if(secAddr > ~secSize) return -14;

			// Range check
			if(secAddr >= addr && secAddr + secSize <= addr + size)
			{
				allowed = true;
				break;
			}
		}
		if(!allowed) return -15;

		if(!skipHashCheck)
		{
			u32 hash[8];
			sha((u32*)(FIRM_LOAD_ADDR + secOffset), secSize, hash,
			    SHA_INPUT_BIG | SHA_MODE_256, SHA_OUTPUT_BIG);
			if(memcmp(section->hash, hash, 32) != 0) return -16;
		}
	}

	strncpy_s((void*)(ITCM_KERNEL_MIRROR + 0x7490), path, 256, 256);
	((const char**)(ITCM_KERNEL_MIRROR + 0x7470))[0] = ((const char*)(ITCM_KERNEL_MIRROR + 0x7490));

	if(!installMode && firmHdr->reserved2[0] & 1) // Adjust argc/v if screen init flag is set.
	{
		static const struct
		{
			u8 *fb1TopLeft;
			u8 *fb1TopRight;
			u8 *fb1Bottom;
			u8 *fb2TopLeft;
			u8 *fb2TopRight;
			u8 *fb2Bottom;
		} fbs =
		{
			(u8*)FRAMEBUF_TOP_A_1,
			(u8*)FRAMEBUF_TOP_A_1,
			(u8*)FRAMEBUF_SUB_A_1 + 0x17700,
			(u8*)FRAMEBUF_TOP_A_2,
			(u8*)FRAMEBUF_TOP_A_2,
			(u8*)FRAMEBUF_SUB_A_2 + 0x17700
		};

		memcpy((void*)(ITCM_KERNEL_MIRROR + 0x7478), &fbs, sizeof(fbs));
		((const char**)(ITCM_KERNEL_MIRROR + 0x7470))[1] = ((const char*)(ITCM_KERNEL_MIRROR + 0x7478));
		firmLaunchArgc = 2;

		return 1;
	}
	else
	{
		firmLaunchArgc = 1;
		return 0;
	}
}

noreturn void firmLaunch(void)
{
	memcpy((void*)A9_STUB_ENTRY, (const void*)firmLaunchStub, A9_STUB_SIZE);

	__systemDeinit();
	deinitCpu();

	((void (*)(int, const char**))A9_STUB_ENTRY)(firmLaunchArgc, (const char**)(ITCM_KERNEL_MIRROR + 0x7470));
	while(1);
}
