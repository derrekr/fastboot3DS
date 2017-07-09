#include "types.h"
#include "util.h"
#include "arm11/i2c.h"
#include "cache.h"
#include "arm11/interrupt.h"



bool battery_ok(void)
{
	u8 state;
	
	if(!I2C_readRegBuf(I2C_DEV_MCU, 0xF, &state, 1))
		return false;
	
	// battery charging, this should be fine
	if((state >> 4) & 1)
		return true;
	
	if(!I2C_readRegBuf(I2C_DEV_MCU, 0xB, &state, 1))
		return false;
	
	if(state >= 10)	// 10% should be enough
		return true;
	
	return false;
}

noreturn void power_off(void)
{
	i2cmcu_lcd_poweroff();

	flushDCache();
	__asm__ __volatile__("cpsid aif" : :);

	I2C_writeReg(I2C_DEV_MCU, 0x20, 1u);

	while(1) waitForInterrupt();
}

noreturn void power_reboot(void)
{
	i2cmcu_lcd_poweroff();

	flushDCache();
	__asm__ __volatile__("cpsid aif" : :);

	I2C_writeReg(I2C_DEV_MCU, 0x20, 1u << 2);

	while(1) waitForInterrupt();
}
