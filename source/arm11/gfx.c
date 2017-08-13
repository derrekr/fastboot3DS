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

/*
 * Based on code from https://github.com/AuroraWright/Luma3DS
 * for compatibility.
 * 
 * Credits go to the Luma3DS devs and derrek for reverse engineering boot11.
*/

#include "types.h"
#include "mem_map.h"
#include "gfx.h"
#include "arm11/i2c.h"
#include "arm11/timer.h"
#include "arm11/interrupt.h"


#define PDN_REGS_BASE            (IO_MEM_ARM9_ARM11 + 0x40000)
#define REG_PDN_GPU_CNT          *((vu32*)(PDN_REGS_BASE + 0x1200))
#define REG_PDN_GPU_CNT2         *((vu32*)(PDN_REGS_BASE + 0x1204))
#define REG_PDN_GPU_CNT2_8BIT    *((vu8* )(PDN_REGS_BASE + 0x1204))
#define REG_PDN_GPU_CNT4         *((vu8* )(PDN_REGS_BASE + 0x1208))
#define REG_PDN_GPU_CNT3         *((vu16*)(PDN_REGS_BASE + 0x1210))

#define LCD_REGS_BASE           (IO_MEM_ARM11_ONLY + 0x2000)
#define REG_LCD_COLORFILL_MAIN  *((vu32*)(LCD_REGS_BASE + 0x204))
#define REG_LCD_COLORFILL_SUB   *((vu32*)(LCD_REGS_BASE + 0xA04))
#define REG_LCD_BACKLIGHT_MAIN  *((vu32*)(LCD_REGS_BASE + 0x240))
#define REG_LCD_BACKLIGHT_SUB   *((vu32*)(LCD_REGS_BASE + 0xA40))

#define GPU_EXT_REGS_BASE       (IO_MEM_ARM11_ONLY + 0x200000)
#define REG_GPU_EXT_CNT         *((vu32*)(GPU_EXT_REGS_BASE + 0x04))



static void gfxSetupLcdTop(void)
{
	*((vu32*)(0x10400400+0x00)) = 0x000001C2;
	*((vu32*)(0x10400400+0x04)) = 0x000000D1;
	*((vu32*)(0x10400400+0x08)) = 0x000001C1;
	*((vu32*)(0x10400400+0x0C)) = 0x000001C1;
	*((vu32*)(0x10400400+0x10)) = 0x00000000;
	*((vu32*)(0x10400400+0x14)) = 0x000000CF;
	*((vu32*)(0x10400400+0x18)) = 0x000000d1;
	*((vu32*)(0x10400400+0x1C)) = 0x01C501C1;
	*((vu32*)(0x10400400+0x20)) = 0x00010000;
	*((vu32*)(0x10400400+0x24)) = 0x0000019D;
	*((vu32*)(0x10400400+0x28)) = 0x00000002;
	*((vu32*)(0x10400400+0x2C)) = 0x00000192;
	*((vu32*)(0x10400400+0x30)) = 0x00000192;
	*((vu32*)(0x10400400+0x34)) = 0x00000192;
	*((vu32*)(0x10400400+0x38)) = 0x00000001;
	*((vu32*)(0x10400400+0x3C)) = 0x00000002;
	*((vu32*)(0x10400400+0x40)) = 0x01960192;
	*((vu32*)(0x10400400+0x44)) = 0x00000000;
	*((vu32*)(0x10400400+0x48)) = 0x00000000;
	*((vu32*)(0x10400400+0x5C)) = (SCREEN_WIDTH_TOP << 16) | SCREEN_HEIGHT_TOP; // Width and height
	*((vu32*)(0x10400400+0x60)) = 0x01C100D1;
	*((vu32*)(0x10400400+0x64)) = 0x01920002;
	*((vu32*)(0x10400400+0x68)) = FRAMEBUF_TOP_A_1;                             // Framebuffer A first address
	*((vu32*)(0x10400400+0x6C)) = FRAMEBUF_TOP_A_2;                             // Framebuffer A second address
	*((vu32*)(0x10400400+0x70)) = 0x00080042;                                   // Format GL_RGB565_OES
	*((vu32*)(0x10400400+0x74)) = 0x00010501;
	*((vu32*)(0x10400400+0x78)) = 0x00000000;                                   // Framebuffer select 0
	*((vu32*)(0x10400400+0x90)) = SCREEN_HEIGHT_TOP * 2;                        // Stride 0
	*((vu32*)(0x10400400+0x94)) = FRAMEBUF_TOP_A_1;                             // Framebuffer B first address
	*((vu32*)(0x10400400+0x98)) = FRAMEBUF_TOP_A_2;                             // Framebuffer B second address
	*((vu32*)(0x10400400+0x9C)) = 0x00000000;

	for(u32 i = 0; i < 0x100; i++)
	{
		u32 val = 0x10101;
		val *= i;
		*((vu32*)(0x10400400+0x84)) = val;
	}
}

static void gfxSetupLcdLow(void)
{
	*((vu32*)(0x10400500+0x00)) = 0x000001C2;
	*((vu32*)(0x10400500+0x04)) = 0x000000D1;
	*((vu32*)(0x10400500+0x08)) = 0x000001C1;
	*((vu32*)(0x10400500+0x0C)) = 0x000001C1;
	*((vu32*)(0x10400500+0x10)) = 0x000000CD;
	*((vu32*)(0x10400500+0x14)) = 0x000000CF;
	*((vu32*)(0x10400500+0x18)) = 0x000000D1;
	*((vu32*)(0x10400500+0x1C)) = 0x01C501C1;
	*((vu32*)(0x10400500+0x20)) = 0x00010000;
	*((vu32*)(0x10400500+0x24)) = 0x0000019D;
	*((vu32*)(0x10400500+0x28)) = 0x00000052;
	*((vu32*)(0x10400500+0x2C)) = 0x00000192;
	*((vu32*)(0x10400500+0x30)) = 0x00000192;
	*((vu32*)(0x10400500+0x34)) = 0x0000004F;
	*((vu32*)(0x10400500+0x38)) = 0x00000050;
	*((vu32*)(0x10400500+0x3C)) = 0x00000052;
	*((vu32*)(0x10400500+0x40)) = 0x01980194;
	*((vu32*)(0x10400500+0x44)) = 0x00000000;
	*((vu32*)(0x10400500+0x48)) = 0x00000011;
	*((vu32*)(0x10400500+0x5C)) = (SCREEN_WIDTH_SUB << 16) | SCREEN_HEIGHT_SUB; // Width and height
	*((vu32*)(0x10400500+0x60)) = 0x01C100D1;
	*((vu32*)(0x10400500+0x64)) = 0x01920052;
	*((vu32*)(0x10400500+0x68)) = FRAMEBUF_SUB_A_1;                             // Framebuffer first address
	*((vu32*)(0x10400500+0x6C)) = FRAMEBUF_SUB_A_2;                             // Framebuffer second address
	*((vu32*)(0x10400500+0x70)) = 0x00080002;                                   // Format GL_RGB565_OES
	*((vu32*)(0x10400500+0x74)) = 0x00010501;
	*((vu32*)(0x10400500+0x78)) = 0x00000000;                                   // Framebuffer select 0
	*((vu32*)(0x10400500+0x90)) = SCREEN_HEIGHT_SUB * 2;                        // Stride 0
	*((vu32*)(0x10400500+0x9C)) = 0x00000000;

	for(u32 i = 0; i < 0x100; i++)
	{
		u32 val = 0x10101;
		val *= i;
		*((vu32*)(0x10400500+0x84)) = val;
	}
}

static void gfxSetupFramebuffers(void)
{
	// Top screen
	*((vu32*)(0x10400400+0x5C)) = (SCREEN_WIDTH_TOP << 16) | SCREEN_HEIGHT_TOP; // Width and height
	*((vu32*)(0x10400400+0x68)) = FRAMEBUF_TOP_A_1;                             // Framebuffer A first address
	*((vu32*)(0x10400400+0x6C)) = FRAMEBUF_TOP_A_2;                             // Framebuffer A second address
	*((vu32*)(0x10400400+0x70)) = 0x00080042;                                   // Format GL_RGB565_OES
	*((vu32*)(0x10400400+0x78)) = 0x00000000;                                   // Framebuffer select 0
	*((vu32*)(0x10400400+0x90)) = SCREEN_HEIGHT_TOP * 2;                        // Stride 0
	*((vu32*)(0x10400400+0x94)) = FRAMEBUF_TOP_A_1;                             // Framebuffer B first address
	*((vu32*)(0x10400400+0x98)) = FRAMEBUF_TOP_A_2;                             // Framebuffer B second address

	// Bottom screen
	*((vu32*)(0x10400500+0x5C)) = (SCREEN_WIDTH_SUB << 16) | SCREEN_HEIGHT_SUB; // Width and height
	*((vu32*)(0x10400500+0x68)) = FRAMEBUF_SUB_A_1;                             // Framebuffer first address
	*((vu32*)(0x10400500+0x6C)) = FRAMEBUF_SUB_A_2;                             // Framebuffer second address
	*((vu32*)(0x10400500+0x70)) = 0x00080002;                                   // Format GL_RGB565_OES
	*((vu32*)(0x10400500+0x78)) = 0x00000000;                                   // Framebuffer select 0
	*((vu32*)(0x10400500+0x90)) = SCREEN_HEIGHT_SUB * 2;                        // Stride 0
}

void GX_memoryFill(u64 *buf0a, u32 buf0v, u32 buf0Sz, u32 val0, u64 *buf1a, u32 buf1v, u32 buf1Sz, u32 val1)
{
	vu32 *REGs_PSC0 = (vu32*)0x10400010;
	vu32 *REGs_PSC1 = (vu32*)0x10400020;

	REGs_PSC0[0] = (u32)buf0a>>3;            // Start address
	REGs_PSC0[1] = ((u32)buf0a + buf0Sz)>>3; // End address 
	REGs_PSC0[2] = val0;                     // Fill value
	REGs_PSC0[3] = buf0v | 1u;               // Pattern + start

	REGs_PSC1[0] = (u32)buf1a>>3;            // Start address
	REGs_PSC1[1] = ((u32)buf1a + buf1Sz)>>3; // End address
	REGs_PSC1[2] = val1;                     // Fill value
	REGs_PSC1[3] = buf1v | 1u;               // Pattern + start
}

void GX_displayTransfer(u64 *in, u32 indim, u64 *out, u32 outdim, u32 flags)
{
	vu32 *REGs_TE = (vu32*)0x10400C00;

	REGs_TE[0] = (u32)in>>3;
	REGs_TE[1] = (u32)out>>3;
	REGs_TE[2] = indim;
	REGs_TE[3] = outdim;
	REGs_TE[4] = flags;
	REGs_TE[5] = 0;
	REGs_TE[6] = 1;
}

void GX_textureCopy(u64 *in, u32 indim, u64 *out, u32 outdim, u32 size)
{
	vu32 *REGs_TE = (vu32*)0x10400C00;

	REGs_TE[0] = (u32)in>>3;
	REGs_TE[1] = (u32)out>>3;
	REGs_TE[4] = 1u<<3;
	REGs_TE[5] = 0;
	REGs_TE[8] = size;
	REGs_TE[9] = indim;
	REGs_TE[10] = outdim;
	REGs_TE[6] = 1;
}

void GFX_setBrightness(u32 brightness)
{
	REG_LCD_BACKLIGHT_MAIN = brightness;
	REG_LCD_BACKLIGHT_SUB = brightness;
}

void GFX_swapFramebufs(void)
{
	static u32 activeFb = 0;
	activeFb ^= 1;

	*((vu32*)(0x10400400+0x78)) = activeFb;
	*((vu32*)(0x10400500+0x78)) = activeFb;
}

static void vblankIrqHandler(UNUSED u32 intSource)
{
	GX_textureCopy((u64*)FRAMEBUF_TOP_A_1, 0, (u64*)FRAMEBUF_TOP_A_2, 0, SCREEN_SIZE_TOP + SCREEN_SIZE_SUB);
}

void GFX_init(void)
{
	if(REG_PDN_GPU_CNT != 0x1007F) // Check if screens are already initialized
	{
		REG_PDN_GPU_CNT = 0x1007F;
		*((vu32*)0x10202014) = 0x00000001;
		*((vu32*)0x1020200C) &= 0xFFFEFFFE;
		REG_LCD_COLORFILL_MAIN = 1u<<24;
		REG_LCD_COLORFILL_SUB = 1u<<24;
		REG_LCD_BACKLIGHT_MAIN = 0x30;
		REG_LCD_BACKLIGHT_SUB = 0x30;
		*((vu32*)0x10202244) = 0x1023E;
		*((vu32*)0x10202A44) = 0x1023E;

		gfxSetupLcdTop();
		gfxSetupLcdLow();

		// The GPU mem fill races against the console.
		//GX_memoryFill((u64*)FRAMEBUF_TOP_A_1, 1u<<9, SCREEN_SIZE_TOP + SCREEN_SIZE_SUB, 0,
		//              (u64*)FRAMEBUF_TOP_A_2, 1u<<9, SCREEN_SIZE_TOP + SCREEN_SIZE_SUB, 0);
		I2C_writeReg(I2C_DEV_MCU, 0x22, 0x2A);

		// We must make sure the I2C bus is not used until this finishes
		// otherwise the screens may not turn on on New 3DS.
		TIMER_sleepMs(3);
	}
	else
	{
		REG_LCD_COLORFILL_MAIN = 1u<<24;
		REG_LCD_COLORFILL_SUB = 1u<<24;
		REG_LCD_BACKLIGHT_MAIN = 0x30;
		REG_LCD_BACKLIGHT_SUB = 0x30;
		gfxSetupFramebuffers();
	}

	IRQ_registerHandler(IRQ_PDC0, 14, 0, true, vblankIrqHandler);
	GFX_swapFramebufs();

	REG_LCD_COLORFILL_MAIN = 0;
	REG_LCD_COLORFILL_SUB = 0;
}

void GFX_deinit(void)
{
	IRQ_unregisterHandler(IRQ_PDC0);

	// Temporary Luma workaround
	GX_memoryFill((u64*)FRAMEBUF_TOP_A_1, 1u<<9, SCREEN_SIZE_TOP + SCREEN_SIZE_SUB + 0x2A300, 0,
	              (u64*)FRAMEBUF_TOP_A_2, 1u<<9, SCREEN_SIZE_TOP + SCREEN_SIZE_SUB + 0x2A300, 0);
	*((vu32*)(0x10400400+0x70)) = 0x00080341;            // Format GL_RGB8_OES
	*((vu32*)(0x10400400+0x90)) = SCREEN_HEIGHT_TOP * 3; // Stride 0
	*((vu32*)(0x10400500+0x70)) = 0x00080301;            // Format GL_RGB8_OES
	*((vu32*)(0x10400500+0x90)) = SCREEN_HEIGHT_SUB * 3; // Stride 0
}
