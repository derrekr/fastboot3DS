/*
 * Based on code from https://github.com/AuroraWright/Luma3DS/blob/master/source/screen.c
 * for compatibility.
 * 
 * Credits go to the Luma3DS devs and derrek for reverse engineering boot11.
*/

#include "mem_map.h"
#include "types.h"
#include "util.h"
#include "arm11/i2c.h"
#include "gfx.h"


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



static void gfx_setup_framebuf_top()
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
	*((vu32*)(0x10400400+0x5C)) = (SCREEN_WIDTH_TOP << 16) | SCREEN_HEIGHT_TOP; // framebuf width & height, 400x240 
	*((vu32*)(0x10400400+0x60)) = 0x01C100D1;
	*((vu32*)(0x10400400+0x64)) = 0x01920002;
	*((vu32*)(0x10400400+0x68)) = FRAMEBUF_TOP_A_1;      // framebuf A first address
	*((vu32*)(0x10400400+0x6C)) = FRAMEBUF_TOP_A_2;      // framebuf A second address
	*((vu32*)(0x10400400+0x70)) = 0x00080042;            // Framebuffer format: main screen, GL_RGB565_OES
	*((vu32*)(0x10400400+0x74)) = 0x00010501;
	*((vu32*)(0x10400400+0x78)) = 0x00000000;
	*((vu32*)(0x10400400+0x90)) = SCREEN_HEIGHT_TOP * 2; // framebuf stride, observed to be 0
	*((vu32*)(0x10400400+0x9C)) = 0x00000000;

	for(u32 i = 0; i < 0x100; i++)
	{
		u32 val = 0x10101;
		val *= i;
		*((vu32*)(0x10400400+0x84)) = val;
	}
}

static void gfx_setup_framebuf_low()
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
	*((vu32*)(0x10400500+0x5C)) = (SCREEN_WIDTH_SUB << 16) | SCREEN_HEIGHT_SUB; // framebuf width & height
	*((vu32*)(0x10400500+0x60)) = 0x01C100D1;
	*((vu32*)(0x10400500+0x64)) = 0x01920052;
	*((vu32*)(0x10400500+0x68)) = FRAMEBUF_SUB_A_1;      // framebuf A first address
	*((vu32*)(0x10400500+0x6C)) = FRAMEBUF_SUB_A_2;      // framebuf A second address
	*((vu32*)(0x10400500+0x70)) = 0x00080002;            // Framebuffer format: sub screen, GL_RGB565_OES
	*((vu32*)(0x10400500+0x74)) = 0x00010501;
	*((vu32*)(0x10400500+0x78)) = 0x00000000;
	*((vu32*)(0x10400500+0x90)) = SCREEN_HEIGHT_SUB * 2; // framebuf stride, observed to be 0
	*((vu32*)(0x10400500+0x9C)) = 0x00000000;

	for(u32 i = 0; i < 0x100; i++)
	{
		u32 val = 0x10101;
		val *= i;
		*((vu32*)(0x10400500+0x84)) = val;
	}
}

void gfx_clear_screens(u64 *top, u32 topSize, u64 *sub, u32 subSize)
{
	vu32 *REGs_PSC0 = (vu32*)0x10400010;
	vu32 *REGs_PSC1 = (vu32*)0x10400020;

	REGs_PSC0[0] = (u32)top>>3; // Start address
	REGs_PSC0[1] = ((u32)top + topSize)>>3; // End address 
	REGs_PSC0[2] = 0; // Fill value
	REGs_PSC0[3] = (2u<<8) | 1u; // 32-bit pattern; start

	REGs_PSC1[0] = (u32)sub>>3; // Start address
	REGs_PSC1[1] = ((u32)sub + subSize)>>3; // End address
	REGs_PSC1[2] = 0; //Fill value
	REGs_PSC1[3] = (2u<<8) | 1u; //32-bit pattern; start

	while(REGs_PSC0[3] & 2 || REGs_PSC1[3] & 2);
}

void gfx_init(void)
{
	REG_PDN_GPU_CNT = 0x1007F;
	*((vu32*)0x10202014) = 0x00000001;
	*((vu32*)0x1020200C) &= 0xFFFEFFFE;
	REG_LCD_BACKLIGHT_MAIN = 0x30;
	REG_LCD_BACKLIGHT_SUB = 0x30;
	*((vu32*)0x10202244) = 0x1023E;
	*((vu32*)0x10202A44) = 0x1023E;

	gfx_setup_framebuf_top();
	gfx_setup_framebuf_low();

	REG_LCD_COLORFILL_MAIN = 1u<<24;
	REG_LCD_COLORFILL_SUB = 1u<<24;
	gfx_clear_screens((u64*)FRAMEBUF_TOP_A_1, SCREEN_HEIGHT_TOP * SCREEN_WIDTH_TOP * 2,
	                  (u64*)FRAMEBUF_SUB_A_1, SCREEN_HEIGHT_SUB * SCREEN_WIDTH_SUB * 2);

	i2cmcu_lcd_poweron();
	i2cmcu_lcd_backlight_poweron();
	REG_LCD_COLORFILL_MAIN = 0;
	REG_LCD_COLORFILL_SUB = 0;
}

void gfx_deinit()
{
	/*i2cmcu_lcd_poweroff();

	*(vu32*)0x10202A44 = 0;
	*(vu32*)0x10202244 = 0;
	REG_LCD_BACKLIGHT_MAIN = 0;
	REG_LCD_BACKLIGHT_SUB = 0;
	*(vu32*)0x10202014 = 0;

	REG_PDN_GPU_CNT = 1;*/

	// Temporary Luma workaround
	gfx_clear_screens((u64*)FRAMEBUF_TOP_A_1, SCREEN_HEIGHT_TOP * SCREEN_WIDTH_TOP * 3,
	                  (u64*)(FRAMEBUF_SUB_A_1 + 0x17700), SCREEN_HEIGHT_SUB * SCREEN_WIDTH_SUB * 3 - 0x17700);
	*((vu32*)(0x10400400+0x70)) = 0x00080341;
	*((vu32*)(0x10400400+0x90)) = SCREEN_HEIGHT_TOP * 3;
	*((vu32*)(0x10400500+0x70)) = 0x00080301;
	*((vu32*)(0x10400500+0x90)) = SCREEN_HEIGHT_SUB * 3;
}
