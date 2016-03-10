#include "types.h"
#include "mem_map.h"
#include "util.h"
#include "arm11/i2c.h"
#include "arm11/gfx.h"
#include "arm11/firm_launch_stub.h"



extern void disableCaches(void);

void turn_off(void)
{
	// Tell ARM9 to deinit everything for poweroff.
	*((vu32*)CORE_SYNC_ID) = 2;
	while(*((vu32*)CORE_SYNC_ID) == 2);

	i2cmcu_lcd_poweroff();
	i2cmcu_lcd_backlight_poweroff();
	while(1)
	{
		i2c_writeregdata(3, 0x20, 0xff);
		sleep_wait(0x200000);
	}
}

int main(void)
{
	// Relocate ARM11 stub
	for (int i = 0; i < (0x200 - 8)>>2; i++)
	{
		((u32*)A11_STUB_ENTRY)[i] = ((u32*)firmLaunchStub)[i];
	}

	while(*((vu32*)CORE_SYNC_ID) != 0x544F4F42)
	{
		if(*((vu32*)CORE_SYNC_ID) == 1)
		{
			gfx_init();
			*((vu32*)CORE_SYNC_ID) = 0;
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
	disableCaches();
	stub_entry();

	return 0;
}
