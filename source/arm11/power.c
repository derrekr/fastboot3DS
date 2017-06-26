#include "types.h"
#include "util.h"
#include "arm11/i2c.h"



bool battery_ok(void)
{
	u8 state;
	
	if(!i2cmcu_readregdata(0xF, &state, 1))
		return false;
	
	// battery charging, this should be fine
	if((state >> 4) & 1)
		return true;
	
	if(!i2cmcu_readregdata(0xB, &state, 1))
		return false;
	
	if(state >= 10)	// 10% should be enough
		return true;
	
	return false;
}

void power_off(void)
{
	i2cmcu_lcd_poweroff();
	i2cmcu_lcd_backlight_poweroff();
	
	for(;;)
	{
		i2c_writeregdata(3, 0x20, 1u);
		wait(0x200000);
	}
}

void power_reboot(void)
{
	i2cmcu_lcd_poweroff();
	i2cmcu_lcd_backlight_poweroff();
	
	for(;;)
	{
		i2c_writeregdata(3, 0x20, 1u << 2);
		wait(0x200000);
	}
}
