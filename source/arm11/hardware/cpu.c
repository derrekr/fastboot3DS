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
#include "arm.h"
#include "arm11/start.h"
#include "arm11/hardware/interrupt.h"
#include "arm11/hardware/scu.h"


#define REG_CFG11_SOCINFO              *((vu16*)0x10140FFC)
#define REG_CFG11_MPCORE_CLKCNT        *((vu16*)0x10141300)
#define REG_CFG11_MPCORE_CNT           *((vu16*)0x10141304)
#define REG_UNK_10140400               *((vu8* )0x10140400)
#define REG_UNK_10140410               *((vu32*)0x10140410)
#define REG_CFG11_BOOTROM_OVERLAY_CNT  *((vu8* )0x10140420)
#define REG_CFG11_BOOTROM_OVERLAY_VAL  *((vu32*)0x10140424)
#define REGs_CFG11_MPCORE_BOOTCNT       ((vu8* )0x10141310)



/*static void NAKED core23Entry(void)
{
	__cpsid(aif);
	*((vu32*)0x17E00100) = 1; // REG_CPU_II_CNT

	const u32 cpuId = __getCpuId();
	// Tell core 0 we are here
	if(cpuId == 3) REGs_CFG11_MPCORE_BOOTCNT[3] = 1;
	else           REGs_CFG11_MPCORE_BOOTCNT[2] = 1;

	// Wait for IPI 2 (core 2) or IPI 3 (core 3)
	u32 tmp;
	do
	{
		__wfi();
		tmp = *((vu32*)0x17E0010C);
		*((vu32*)0x17E00110) = tmp;
	} while(tmp != cpuId);

	// Jump to real entrypoint
	_start();
}*/

void core123Init(void)
{
	/*if(REG_CFG11_SOCINFO & 2)
	{
		*((vu32*)0x17E00100) = 1;         // REG_CPU_II_CNT
		*((vu32*)0x17E01288) = 0x1000000; // REGs_GID_PEN_CLR
		*((vu8* )0x17E01458) = 0;         // REGs_GID_IPRIO
		*((vu8* )0x17E01858) = 1;         // REGs_GID_ITARG
		*((vu32*)0x17E01108) = 0x1000000; // REGs_GID_ENA_SET

		u16 clkCnt;
		// If clock modifier is 3x use clock 3x. Also enables FCRAM extension?
		if(REG_CFG11_SOCINFO & 4) clkCnt = 2<<1 | 1;
		else                      clkCnt = 1<<1 | 1;

		if((REG_CFG11_MPCORE_CLKCNT & 7) != clkCnt)
		{
			// Poweron core 3 if available
			if(REG_CFG11_SOCINFO & 4) REG_CFG11_MPCORE_CNT = 0x101; // Poweron core 2 and 3
			else                      REG_CFG11_MPCORE_CNT = 1;     // Poweron core 2

			//waitCycles(403);
			// Necessary delay
			for(vu32 i = 403; i > 0; i--);

			REG_CFG11_MPCORE_CLKCNT = (clkCnt & 7) | 0x8000;
			do
			{
				__wfi();
			} while(!(REG_CFG11_MPCORE_CLKCNT & 0x8000));
			*((vu32*)0x17E01288) = 0x1000000; // REGs_GID_PEN_CLR
			REG_UNK_10140400 = 3;   // Clock related?
		}
		REG_UNK_10140410 = 0x3FFFF; // Clock related?

		if((REG_SCU_CONFIG & 3) == 3)
		{
			// Set core 2/3 to normal mode (running)
			REG_SCU_CPU_STAT &= ~0xF0;

			const u16 clkCnt = REG_CFG11_MPCORE_CLKCNT & 7;
			u16 tmpClkCnt;
			if(REG_CFG11_SOCINFO & 4) tmpClkCnt = 1;
			else                      tmpClkCnt = 2;

			if(clkCnt != tmpClkCnt)
			{
				REG_CFG11_MPCORE_CLKCNT = (tmpClkCnt & 7) | 0x8000;
				do
				{
					__wfi();
				} while(!(REG_CFG11_MPCORE_CLKCNT & 0x8000));
				*((vu32*)0x17E01288) = 0x1000000; // REGs_GID_PEN_CLR
			}

			REG_CFG11_BOOTROM_OVERLAY_CNT = 1;
			REG_CFG11_BOOTROM_OVERLAY_VAL = (u32)core23Entry;
			// If not already done eable instruction and data overlays
			if(!(REGs_CFG11_MPCORE_BOOTCNT[2] & 0x10)) REGs_CFG11_MPCORE_BOOTCNT[2] = 3;
			if(!(REGs_CFG11_MPCORE_BOOTCNT[3] & 0x10)) REGs_CFG11_MPCORE_BOOTCNT[3] = 3;
			// Wait for core 2/3 to jump out of boot11
			while((REGs_CFG11_MPCORE_BOOTCNT[2] & 0x12) != 0x10);
			while((REGs_CFG11_MPCORE_BOOTCNT[3] & 0x12) != 0x10);
			REG_CFG11_BOOTROM_OVERLAY_CNT = 0; // Disable all overlays

			// Set clock back to original one
			if(clkCnt != tmpClkCnt)
			{
				REG_CFG11_MPCORE_CLKCNT = (clkCnt & 7) | 0x8000;
				do
				{
					__wfi();
				} while(!(REG_CFG11_MPCORE_CLKCNT & 0x8000));
			}
		}

		*((vu32*)0x17E01188) = 0x1000000; // REGs_GID_ENA_CLR

		// Wakeup core 2/3 and let them jump to their entrypoint.
		*((vu32*)0x17E01F00) = 0b0100<<16 | 2;
		*((vu32*)0x17E01F00) = 0b1000<<16 | 3;
	}*/

	// Wakeup core 1
	*((vu32*)0x1FFFFFDC) = (u32)_start;  // Core 1 entrypoint
	*((vu32*)0x17E01F00) = 0b10<<16 | 1;
}
