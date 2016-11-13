#include "types.h"
#include "IO.h"
#include "mem_map.h"
#include "util.h"
#include "arm11/i2c.h"
#include "gfx.h"



extern void gpio_set_bit(vu16 *reg, u8 bit_num);
extern void gpio_clear_bit(vu16 *reg, u8 bit_num);

void gfx_setup_framebuf_top()
{
	*((vu32 *)(0x10400400)) = 0x1C2;
	*((vu32 *)(0x10400400+0x4)) = 0xD1;
	*((vu32 *)(0x10400400+0xC)) = 0x1C1;
	*((vu32 *)(0x10400400+0x10)) = 0x0;
	*((vu32 *)(0x10400400+0x60)) = 0x1C100D1;
	*((vu32 *)(0x10400400+0x14)) = 0xCF;
	*((vu32 *)(0x10400400+0x18)) = 0xD1;
	*((vu32 *)(0x10400400+0x8)) = 0x1C1;
	*((vu32 *)(0x10400400+0x1C)) = 0x1C501C1;
	*((vu32 *)(0x10400400+0x20)) = 0x10000;
	*((vu32 *)(0x10400400+0x24)) = 0x19D;
	*((vu32 *)(0x10400400+0x28)) = 0x02;
	*((vu32 *)(0x10400400+0x30)) = 0x192;
	*((vu32 *)(0x10400400+0x34)) = 0x192;
	*((vu32 *)(0x10400400+0x64)) = 0x1920002;
	*((vu32 *)(0x10400400+0x38)) = 0x01;
	*((vu32 *)(0x10400400+0x3C)) = 0x02;
	*((vu32 *)(0x10400400+0x2C)) = 0x192;
	*((vu32 *)(0x10400400+0x40)) = 0x1930192;
	*((vu32 *)(0x10400400+0x44)) = 0;
	*((vu32 *)(0x10400400+0x48)) = 0x0;
	*((vu32 *)(0x10400400+0x5C)) = 0x19000F0; // framebuf width & height, 400x240 
	*((vu32 *)(0x10400400+0x90)) = 0x1E0; // 0x3C0; // framebuf stride, observed to be 0
	*((vu32 *)(0x10400400+0x68)) = FRAMEBUF_TOP_A_1; // framebuf A first address
	*((vu32 *)(0x10400400+0x6C)) = FRAMEBUF_TOP_A_2; // framebuf A second address
	*((vu32 *)(0x10400400+0x9C)) = 0x1920000;
	*((vu32 *)(0x10400400+0x70)) = 0x80042; // 0x80040;	// Framebuffer format: main screen, GL_RGBA8_OES
	*((vu32 *)(0x10400400+0x74)) = 0x10700; // gets set to 0x10501 later on
	*((vu32 *)(0x10400400+0x78)) = 0x70100; // gets set to 0x0 later on
	*((vu32 *)(0x10400400+0x80)) = 0x0;

	for(u32 i = 0; i < 0x100; i++)
	{
		u32 val = 0x10101;
		val *= i;
		*((vu32*)(0x10400400+0x84)) = val;
	}
}

void gfx_setup_framebuf_low()
{
	*((vu32 *)(0x10400500)) = 0x1C2;
	*((vu32 *)(0x10400500+0x4)) = 0xD1;
	*((vu32 *)(0x10400500+0xC)) = 0x1C1;
	*((vu32 *)(0x10400500+0x10)) = 0xCD;
	*((vu32 *)(0x10400500+0x60)) = 0x1C100D1;
	*((vu32 *)(0x10400500+0x14)) = 0xCF;
	*((vu32 *)(0x10400500+0x18)) = 0xD1;
	*((vu32 *)(0x10400500+0x8)) = 0x1C1;
	*((vu32 *)(0x10400500+0x1C)) = 0x1C501C1;
	*((vu32 *)(0x10400500+0x20)) = 0x10000;
	*((vu32 *)(0x10400500+0x24)) = 0x19D;
	*((vu32 *)(0x10400500+0x28)) = 0x52;
	*((vu32 *)(0x10400500+0x30)) = 0x192;
	*((vu32 *)(0x10400500+0x34)) = 0x4F;
	*((vu32 *)(0x10400500+0x64)) = 0x1920052;
	*((vu32 *)(0x10400500+0x38)) = 0x50;
	*((vu32 *)(0x10400500+0x3C)) = 0x52;
	*((vu32 *)(0x10400500+0x2C)) = 0x192;
	*((vu32 *)(0x10400500+0x40)) = 0x1930192;
	*((vu32 *)(0x10400500+0x44)) = 0;
	*((vu32 *)(0x10400500+0x48)) = 0x11;
	*((vu32 *)(0x10400500+0x5C)) = 0x14000F0; // framebuf width & height
	*((vu32 *)(0x10400500+0x90)) = 0x1E0; // 0x3C0; // framebuf stride, observed to be 0
	*((vu32 *)(0x10400500+0x68)) = FRAMEBUF_SUB_A_1; // framebuf A first address
	*((vu32 *)(0x10400500+0x6C)) = FRAMEBUF_SUB_A_2; // framebuf A second address
	*((vu32 *)(0x10400500+0x9C)) = 0x1920000;
	*((vu32 *)(0x10400500+0x70)) = 0x80002; // 0x80000;	// Framebuffer format: sub screen, GL_RGBA8_OES
	*((vu32 *)(0x10400500+0x74)) = 0x10700; // gets set to 0x10501 later on
	*((vu32 *)(0x10400500+0x78)) = 0x70100; // gets set to 0x0 later on
	*((vu32 *)(0x10400500+0x80)) = 0x0;
	
	for(u32 i = 0; i < 0x100; i++)
	{
		u32 val = 0x10101;
		val *= i;
		*((vu32*)(0x10400500+0x84)) = val;
	}
}

void gfx_init_framebufsetup()
{
	REG_PDN_GPU_CNT2_8BIT |= 1;	// turn on GPU
	REG_PDN_GPU_CNT |= 1<<0x10;	// turn on LCD backlight
	wait(0x1000);
	
	REG_PDN_GPU_CNT |= 0x1007F;
	REG_PDN_GPU_CNT4 |= 1;
	
	REG_GPU_EXT_CNT |= 0x300;
	
	//memset(VRAM_BASE, 0xFF, VRAM_SIZE);
	//gfx_set_framebufs(0x00, 0x00, 0x00, 0x00);
	
	gfx_setup_framebuf_top();
	gfx_setup_framebuf_low();
	
	*((vu32 *)(0x1040044C)) = 0xFF00;
	*((vu32 *)(0x1040054C)) = 0xFF;

	
	wait(0x8000);
	*((vu32 *)(0x10400578)) = 0x0;
	*((vu32 *)(0x10400478)) = 0x0;
	*((vu32 *)(0x10400574)) = 0x10501;
	*((vu32 *)(0x10400474)) = 0x10501;
}

void gpio_stuff_bit7_bit10()
{
	// gpio_stuff
	*((vu32 *) 0x10202014) = 0;
	if(((*((u16 *) 0x10140FFC)) << 0x1F) >> 0x1F)
	{
		gpio_set_bit((vu16 *)0x10147022, 0xA);
		gpio_clear_bit((vu16 *)0x10147020, 0xA);
	}
	
	// gpio_stuff2
	if(((*((vu16 *) 0x10140FFC)) << 0x1F) >> 0x1F)
	{
		gpio_set_bit((vu16 *)0x10147022, 7);
		gpio_clear_bit((vu16 *)0x10147020, 7);
	}
}

void gfx_lcd_set_mcu_conf()
{
	u8 mcu_data;
	if(i2cmcu_readreg0x0_upper(&mcu_data) && (mcu_data != 0x01))
	{
		*((vu32 *) 0x10202244) = (*((vu32 *) 0x10202244) & 0xFFFF0C00) + 0x60FF;
		*((vu32 *) 0x10202A44) = (*((vu32 *) 0x10202A44) & 0xFFFF0C00) + 0x60FF;
	}
	else
	{
		*((vu32 *) 0x10202244) = (*((vu32 *) 0x10202244) & 0xFFFF0C00) + 0xFF;
		*((vu32 *) 0x10202A44) = (*((vu32 *) 0x10202A44) & 0xFFFF0C00) + 0xFF;
	}
}

void gfx_init_step1()
{
	gfx_init_framebufsetup();
	*((vu32 *) 0x10202000) = 0;
	*((vu32 *) 0x10202004) = 0xA390A39;
	//gpio_stuff_bit7_bit10();
	gfx_lcd_set_mcu_conf();
}

u32 gfx_i2c_lcd_init()
{
	if(!i2c_write_regdata_dev5_dev6(true, true, 0x11, 0x10) ||
		!i2c_write_regdata_dev5_dev6(true, false, 0x50, 0x1) ||
		!i2c_write_regdata_dev5_dev6(true, true, 0x60, 0) ||
		!i2c_write_regdata_dev5_dev6(true, true, 1, 0x10))
		return 0;
	return 1;
}

void gfx_init_step2()
{
	wait(0x1000);
	// gpio_stuff
	*((vu32 *) 0x10202014) = 1;
	if(((*((vu16 *) 0x10140FFC)) << 0x1F) >> 0x1F)
	{
		gpio_set_bit((vu16 *)0x10147022, 0xA);
		gpio_set_bit((vu16 *)0x10147020, 0xA);
	}
	
	// enable color fill regs for both screens
	REG_LCD_COLORFILL_MAIN |= 1<<24;
	REG_LCD_COLORFILL_SUB |= 1<<24;
	
	*((vu32 *) 0x1020200C) &= 0xFFFEFFFE;
	
	wait(0x1000);
	//gfx_i2c_lcd_init();
	i2cmcu_lcd_poweron();
}

u32 gfx_i2c_lcd_finish()
{
	u8 response = i2c_write_echo_read(5, 0x40, 0x62);
	if(i2c_write_echo_read(6, 0x40, 0x62) != response)
		return 0;
	if(response == 1)
		return 1;
	return 0;
}

void gfx_init_step3()
{
	REG_LCD_BACKLIGHT_MAIN = 0x0F; // Equals home menu brightness level 1
	REG_LCD_BACKLIGHT_SUB = 0x0F;
	
	int i;
	for(i=0; i<10; i++)
	{
		//if(!gfx_i2c_lcd_finish())
			wait(0x20000);
		//else break;
	}
	
	REG_LCD_COLORFILL_MAIN = 0;
	REG_LCD_COLORFILL_SUB = 0;
	
	*((vu32 *) 0x10202244) = *((vu32 *) 0x10202244) | 0x10000;
	*((vu32 *) 0x10202A44) = *((vu32 *) 0x10202A44) | 0x10000;
	
	// enable lcd lights
	i2cmcu_lcd_backlight_poweron();
}

void gfx_init()
{
	static bool gfxInitDone;

	if(gfxInitDone)
		return;

	gfxInitDone = true;

	gfx_init_step1();
	gfx_init_step2();
	gfx_init_step3();
}

void gfx_set_framebufs(u8 r, u8 g, u8 b, u8 a)
{
	u8 *framebuf = (u8 *) FRAMEBUF_TOP_A_1;
	for(u32 i=0; i<SCREEN_HEIGHT_TOP*SCREEN_WIDTH_TOP; i++)
	{
		*framebuf++ = a;
		*framebuf++ = b;
		*framebuf++ = g;
		*framebuf++ = r;
	}
	
	framebuf = (u8 *) FRAMEBUF_SUB_A_1;
	for(u32 i=0; i<SCREEN_HEIGHT_SUB*SCREEN_WIDTH_SUB; i++)
	{
		*framebuf++ = a;
		*framebuf++ = b;
		*framebuf++ = g;
		*framebuf++ = r;
	}
}

void gfx_set_black_sub()
{
	REG_LCD_COLORFILL_SUB |= 1<<24;
}

void gfx_clear_framebufs()
{
	gfx_set_framebufs(0x00, 0x00, 0x00, 0x00);
}

