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

#include <stdio.h>
#include <string.h>
#include "types.h"
#include "mem_map.h"
#include "hardware/pxi.h"
#include "fatfs/ff.h"
#include "fs.h"
#include "arm9/hardware/interrupt.h"
#include "hardware/gfx.h"
#include "arm9/hardware/ndma.h"

static u32 debugHash = 0;


void debugHashCodeRoData()
{
#ifndef NDEBUG
	extern u32 *__start__;
	extern u32 *__exidx_start;
	
	u32 *start = __start__;
	u32 *end = __exidx_start;
	
	debugHash = 0;
	
	for(u32 *ptr = start; ptr < end; ptr++)
		debugHash += *ptr;
#endif
}

noreturn void panic()
{
	register u32 lr __asm__("lr");

	enterCriticalSection();

	fsDeinit();
	//PXI_sendWord(PXI_CMD_ALLOW_POWER_OFF);

	while(1)
	{
		const u32 color = RGB8_to_565(255, 0, 0)<<16 | RGB8_to_565(255, 0, 0);
		NDMA_fill((u32*)FRAMEBUF_SUB_A_1, color, SCREEN_SIZE_SUB);
		NDMA_fill((u32*)FRAMEBUF_SUB_A_2, color, SCREEN_SIZE_SUB);
	}
}

noreturn void panicMsg(const char *msg)
{
	register u32 lr __asm__("lr");

	enterCriticalSection();

	fsDeinit();
	//PXI_sendWord(PXI_CMD_ALLOW_POWER_OFF);

	while(1)
	{
		const u32 color = RGB8_to_565(255, 0, 0)<<16 | RGB8_to_565(255, 0, 0);
		NDMA_fill((u32*)FRAMEBUF_SUB_A_1, color, SCREEN_SIZE_SUB);
		NDMA_fill((u32*)FRAMEBUF_SUB_A_2, color, SCREEN_SIZE_SUB);
	}
}

// Expects the registers in the exception stack to be in the following order:
// r0-r14, pc (unmodified), cpsr
noreturn void guruMeditation(u8 type, const u32 *excStack)
{
	const char *const typeStr[3] = {"Undefined instruction", "Prefetch abort", "Data abort"};
	u32 realPc, instSize = 4;
	bool codeChanged = false;


	// verify text and rodata
	u32 prevHash = debugHash;
	debugHashCodeRoData();
	if(prevHash != debugHash)
		codeChanged = true;

	//consoleInit(0, NULL, true);

	if(excStack[16] & 0x20) instSize = 2; // Processor was in Thumb mode?
	if(type == 2) realPc = excStack[15] - (instSize * 2); // Data abort
	else realPc = excStack[15] - instSize; // Other

	/*ee_printf("\x1b[41m\x1b[0J\x1b[9CGuru Meditation Error!\n\n%s:\n", typeStr[type]);
	ee_printf("CPSR: 0x%08" PRIX32 "\n"
	       "r0 = 0x%08" PRIX32 " r8  = 0x%08" PRIX32 "\n"
	       "r1 = 0x%08" PRIX32 " r9  = 0x%08" PRIX32 "\n"
	       "r2 = 0x%08" PRIX32 " r10 = 0x%08" PRIX32 "\n"
	       "r3 = 0x%08" PRIX32 " r11 = 0x%08" PRIX32 "\n"
	       "r4 = 0x%08" PRIX32 " r12 = 0x%08" PRIX32 "\n"
	       "r5 = 0x%08" PRIX32 " sp  = 0x%08" PRIX32 "\n"
	       "r6 = 0x%08" PRIX32 " lr  = 0x%08" PRIX32 "\n"
	       "r7 = 0x%08" PRIX32 " pc  = 0x%08" PRIX32 "\n\n",
	       excStack[16],
	       excStack[0], excStack[8],
	       excStack[1], excStack[9],
	       excStack[2], excStack[10],
	       excStack[3], excStack[11],
	       excStack[4], excStack[12],
	       excStack[5], excStack[13],
	       excStack[6], excStack[14],
	       excStack[7], realPc);

	ee_puts("Stack dump:");

	u32 sp = excStack[13];
	if(sp >= DTCM_BASE && sp < DTCM_BASE + DTCM_SIZE && !(sp & 3u))
	{
		u32 stackWords = ((DTCM_BASE + DTCM_SIZE - sp) / 4 > 45 ? 45 : (DTCM_BASE + DTCM_SIZE - sp) / 4);

		u32 newlineCounter = 0;
		for(u32 i = 0; i < stackWords; i++)
		{
			if(newlineCounter == 3) {ee_printf("\n"); newlineCounter = 0;}
			ee_printf("0x%08" PRIX32 " ", ((u32*)sp)[i]);
			newlineCounter++;
		}
	}

	if(codeChanged) ee_printf("Attention: RO section data changed!!");*/

	// avoid fs corruptions
	fsDeinit();
	//PXI_sendWord(PXI_CMD_ALLOW_POWER_OFF);

	while(1) __wfi();
}

void dumpMem(u8 *mem, u32 size, char *filepath)
{
	FIL file;
	UINT bytesWritten;

	if(f_open(&file, filepath, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
		return;
		
	f_write(&file, mem, size, &bytesWritten);
	
	f_sync(&file);

	f_close(&file);
}
