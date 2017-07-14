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
