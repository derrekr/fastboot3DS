#include "types.h"
#include "util.h"
#include "arm11/i2c.h"
#include "cache.h"
#include "arm11/interrupt.h"



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

noreturn void power_off(void)
{
	i2cmcu_lcd_poweroff();
	i2cmcu_lcd_backlight_poweroff();

	flushDCache();
	__asm__ __volatile__("cpsid aif" : :);

	i2c_writeregdata(3, 0x20, 1u);

	while(1) waitForInterrupt();
}

noreturn void power_reboot(void)
{
	i2cmcu_lcd_poweroff();
	i2cmcu_lcd_backlight_poweroff();

	flushDCache();
	__asm__ __volatile__("cpsid aif" : :);

	i2c_writeregdata(3, 0x20, 1u << 2);

	while(1) waitForInterrupt();
}
