#include <string.h>
#include <stdbool.h>
#include "types.h"
#include "mem_map.h"
#include "util.h"
#include "arm11/i2c.h"
#include "gfx.h"



void turn_off(void)
{
	// Tell ARM9 to deinit everything for poweroff.
	CORE_SYNC_VAL = 2;
	while(CORE_SYNC_VAL == 2);

	i2cmcu_lcd_poweroff();
	i2cmcu_lcd_backlight_poweroff();
	while(true)
	{
		i2c_writeregdata(3, 0x20, 0xff);
		sleep_wait(0x200000);
	}
}

void firmLaunchStub(void)
{
	// Answer ARM0
	CORE_SYNC_VAL = 0x4F4B4F4B;

	// Wait for entry address
	while(!A11_ENTRY_VAL);

	// Tell ARM9 we got the entry
	CORE_SYNC_VAL = 0x544F4F42;

	void (*arm11_entry)(void) = (void*)A11_ENTRY_VAL;
	//for(u32 i = (240*320>>1); i < 240*320; i++) ((u32*)0x18300000)[i] = 0x0000FFFF;
	arm11_entry();
}

int main(void)
{
	// Relocate ARM11 stub
	for (int i = 0; i < (0x200 - 8)>>2; i++)
	{
		((u32*)A11_STUB_ENTRY)[i] = ((u32*)firmLaunchStub)[i];
	}

	while(CORE_SYNC_VAL != 0x544F4F42)
	{
		if(CORE_SYNC_VAL == 1)
		{
			gfx_init();
			CORE_SYNC_VAL = 0;
		}
		u8 hidstate = i2cmcu_readreg_hid();

		if(hidstate & MCU_HID_POWER_BUTTON_PRESSED)
			turn_off();
		if(hidstate & MCU_HID_SHELL_GOT_CLOSED)
			i2cmcu_lcd_poweroff();
		else if(hidstate & MCU_HID_SHELL_GOT_OPENED)
		{
			i2cmcu_lcd_poweron();
			i2cmcu_lcd_backlight_poweron();
		}
		if(hidstate & MCU_HID_HOME_BUTTON_PRESSED)
			i2cmcu_lcd_backlight_poweroff();
		sleep_wait(0xC000);
	}

	// Turn off LCDs and backlight before FIRM launch.
	i2cmcu_lcd_poweroff();
	i2cmcu_lcd_backlight_poweroff();

	void (*stub_entry)(void) = (void*)A11_STUB_ENTRY;
	stub_entry();

	return 0;
}
