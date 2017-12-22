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
#include "hardware/gfx.h"
#include "arm11/hardware/mcu.h"
#include "arm11/hardware/interrupt.h"
#include "arm.h"


#define PDN_REGS_BASE           (IO_MEM_ARM9_ARM11 + 0x40000)
#define REG_PDN_GPU_CNT         *((vu32*)(PDN_REGS_BASE + 0x1200))
#define REG_PDN_GPU_CNT2        *((vu32*)(PDN_REGS_BASE + 0x1204))
#define REG_PDN_GPU_CNT2_8BIT   *((vu8* )(PDN_REGS_BASE + 0x1204))
#define REG_PDN_GPU_CNT4        *((vu8* )(PDN_REGS_BASE + 0x1208))
#define REG_PDN_GPU_CNT3        *((vu16*)(PDN_REGS_BASE + 0x1210))

#define LCD_REGS_BASE           (IO_MEM_ARM11_ONLY + 0x2000)
#define REG_LCD_COLORFILL_MAIN  *((vu32*)(LCD_REGS_BASE + 0x204))
#define REG_LCD_COLORFILL_SUB   *((vu32*)(LCD_REGS_BASE + 0xA04))
#define REG_LCD_BACKLIGHT_MAIN  *((vu32*)(LCD_REGS_BASE + 0x240))
#define REG_LCD_BACKLIGHT_SUB   *((vu32*)(LCD_REGS_BASE + 0xA40))

#define GPU_EXT_REGS_BASE       (IO_MEM_ARM11_ONLY + 0x200000)
#define REG_GPU_EXT_CNT         *((vu32*)(GPU_EXT_REGS_BASE + 0x0004))

#define REGs_PSC0                ((vu32*)(GPU_EXT_REGS_BASE + 0x0010))
#define REGs_PSC1                ((vu32*)(GPU_EXT_REGS_BASE + 0x0020))

#define REGs_TRANS_ENGINE        ((vu32*)(GPU_EXT_REGS_BASE + 0x0C00))


static u32 activeFb = 0;
static volatile bool eventTable[6] = {0};



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

static void gfxSetupLcdSub(void)
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
	if(buf0a)
	{
		REGs_PSC0[0] = (u32)buf0a>>3;            // Start address
		REGs_PSC0[1] = ((u32)buf0a + buf0Sz)>>3; // End address 
		REGs_PSC0[2] = val0;                     // Fill value
		REGs_PSC0[3] = buf0v | 1u;               // Pattern + start
	}

	if(buf1a)
	{
		REGs_PSC1[0] = (u32)buf1a>>3;            // Start address
		REGs_PSC1[1] = ((u32)buf1a + buf1Sz)>>3; // End address
		REGs_PSC1[2] = val1;                     // Fill value
		REGs_PSC1[3] = buf1v | 1u;               // Pattern + start
	}
}

void GX_displayTransfer(u64 *in, u32 indim, u64 *out, u32 outdim, u32 flags)
{
	if(!in || !out) return;

	REGs_TRANS_ENGINE[0] = (u32)in>>3;
	REGs_TRANS_ENGINE[1] = (u32)out>>3;
	REGs_TRANS_ENGINE[2] = indim;
	REGs_TRANS_ENGINE[3] = outdim;
	REGs_TRANS_ENGINE[4] = flags;
	REGs_TRANS_ENGINE[5] = 0;
	REGs_TRANS_ENGINE[6] = 1;
}

// Example: GX_textureCopy(in, (240 * 2)<<12 | (240 * 2)>>4, out, (240 * 2)<<12 | (240 * 2)>>4, 240 * 400);
// Copies every second line of a 240x400 framebuffer.
void GX_textureCopy(u64 *in, u32 indim, u64 *out, u32 outdim, u32 size)
{
	if(!in || !out) return;

	REGs_TRANS_ENGINE[0] = (u32)in>>3;
	REGs_TRANS_ENGINE[1] = (u32)out>>3;
	REGs_TRANS_ENGINE[4] = 1u<<3;
	REGs_TRANS_ENGINE[8] = size;
	REGs_TRANS_ENGINE[9] = indim;
	REGs_TRANS_ENGINE[10] = outdim;
	REGs_TRANS_ENGINE[6] = 1;
}

void GFX_setBrightness(u32 top, u32 sub)
{
	REG_LCD_BACKLIGHT_MAIN = top;
	REG_LCD_BACKLIGHT_SUB = sub;
}

void* GFX_getFramebuffer(u8 screen)
{
	static void *const framebufTable[2][2] =
	{
		{(void*)FRAMEBUF_SUB_A_2, (void*)FRAMEBUF_SUB_A_1},
		{(void*)FRAMEBUF_TOP_A_2, (void*)FRAMEBUF_TOP_A_1}
	};

	return framebufTable[screen][activeFb];
}

void GFX_swapFramebufs(void)
{
	activeFb ^= 1;

	*((vu32*)(0x10400400+0x78)) = activeFb;
	*((vu32*)(0x10400500+0x78)) = activeFb;
}

static void gfxIrqHandler(u32 intSource)
{
	eventTable[intSource - IRQ_PSC0] = true;
}

void GFX_waitForEvent(GfxEvent event, bool discard)
{
	if(discard) eventTable[event] = false;
	while(!eventTable[event]) __wfe();
	eventTable[event] = false;
}

void GFX_init(void)
{
	if(REG_PDN_GPU_CNT != 0x1007F) // Check if screens are already initialized
	{
		REG_PDN_GPU_CNT = 0x1007F;
		*((vu32*)0x10202014) = 0x00000001;
		*((vu32*)0x1020200C) &= 0xFFFEFFFE;
		REG_LCD_COLORFILL_MAIN = 1u<<24; // Force blackscreen
		REG_LCD_COLORFILL_SUB = 1u<<24;  // Force blackscreen
		GFX_setBrightness(DEFAULT_BRIGHTNESS, DEFAULT_BRIGHTNESS);
		*((vu32*)0x10202244) = 0x1023E;
		*((vu32*)0x10202A44) = 0x1023E;

		gfxSetupLcdTop();
		gfxSetupLcdSub();

		MCU_powerOnLCDs(); // Power on LCDs and backlight
	}
	else
	{
		REG_LCD_COLORFILL_MAIN = 1u<<24; // Force blackscreen
		REG_LCD_COLORFILL_SUB = 1u<<24;  // Force blackscreen
		GFX_setBrightness(DEFAULT_BRIGHTNESS, DEFAULT_BRIGHTNESS);
		gfxSetupFramebuffers();
	}

	IRQ_registerHandler(IRQ_PSC0, 14, 0, true, gfxIrqHandler);
	IRQ_registerHandler(IRQ_PSC1, 14, 0, true, gfxIrqHandler);
	IRQ_registerHandler(IRQ_PDC0, 14, 0, true, gfxIrqHandler);
	IRQ_registerHandler(IRQ_PPF, 14, 0, true, gfxIrqHandler);
	//IRQ_registerHandler(IRQ_P3D, 14, 0, true, gfxIrqHandler);

	// Warning. The GPU mem fill races against the console.
	GX_memoryFill((u64*)FRAMEBUF_TOP_A_1, 1u<<9, SCREEN_SIZE_TOP + SCREEN_SIZE_SUB, 0,
	              (u64*)FRAMEBUF_TOP_A_2, 1u<<9, SCREEN_SIZE_TOP + SCREEN_SIZE_SUB, 0);
	GFX_waitForEvent(GFX_EVENT_PSC1, true);
	GX_memoryFill((u64*)RENDERBUF_TOP, 1u<<9, SCREEN_SIZE_TOP, 0, (u64*)RENDERBUF_SUB, 1u<<9, SCREEN_SIZE_SUB, 0);
	GFX_waitForEvent(GFX_EVENT_PSC0, true);

	// On certain N3DS consoles the first framebuffer swap on cold boot
	// can cause graphics corruption if we don't wait for the first VBlank.
	// This also fixes the problem of the screens not turning on on N3DS.
	GFX_waitForEvent(GFX_EVENT_PDC0, true);

	REG_LCD_COLORFILL_MAIN = 0;
	REG_LCD_COLORFILL_SUB = 0;
}

void GFX_enterLowPowerState(void)
{
	REG_LCD_COLORFILL_MAIN = 1u<<24; // Force blackscreen
	REG_LCD_COLORFILL_SUB = 1u<<24;  // Force blackscreen
	GFX_waitForEvent(GFX_EVENT_PDC0, true);
	GFX_deinit(false);
}

void GFX_returnFromLowPowerState(void)
{
	GFX_init();
}

void GFX_deinit(bool keepLcdsOn)
{
	IRQ_disable(IRQ_PSC0);
	IRQ_disable(IRQ_PSC1);
	IRQ_disable(IRQ_PDC0);
	IRQ_disable(IRQ_PPF);
	//IRQ_disable(IRQ_P3D);

	if(keepLcdsOn)
	{
		// Temporary Luma workaround
		GX_memoryFill((u64*)FRAMEBUF_TOP_A_1, 1u<<9, SCREEN_SIZE_TOP + SCREEN_SIZE_SUB + 0x2A300, 0,
		              (u64*)FRAMEBUF_TOP_A_2, 1u<<9, SCREEN_SIZE_TOP + SCREEN_SIZE_SUB + 0x2A300, 0);
		*((vu32*)(0x10400400+0x70)) = 0x00080341;                 // Format GL_RGB8_OES
		*((vu32*)(0x10400400+0x78)) = 0;                          // Select first framebuffer
		*((vu32*)(0x10400400+0x90)) = SCREEN_HEIGHT_TOP * 3;      // Stride 0
		*((vu32*)(0x10400500+0x68)) = FRAMEBUF_SUB_A_1 + 0x17700; // Sub framebuffer first address
		*((vu32*)(0x10400500+0x6C)) = FRAMEBUF_SUB_A_2 + 0x17700; // Sub framebuffer second address
		*((vu32*)(0x10400500+0x70)) = 0x00080301;                 // Format GL_RGB8_OES
		*((vu32*)(0x10400500+0x78)) = 0;                          // Select first framebuffer
		*((vu32*)(0x10400500+0x90)) = SCREEN_HEIGHT_SUB * 3;      // Stride 0
	}
	else
	{
		// This deinits the GPU correctly but it is broken with Luma.
		MCU_powerOffLCDs();
		GFX_setBrightness(0, 0);
		*((vu32*)0x10202244) = 0;
		*((vu32*)0x10202A44) = 0;
		*((vu32*)0x1020200C) = 0x10001;
		*((vu32*)0x10202014) = 0;
		REG_PDN_GPU_CNT = 0x10001;
	}
}
