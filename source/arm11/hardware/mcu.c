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
 *  Based on code from https://github.com/smealum/ctrulib
 */

#include "mem_map.h"
#include "arm11/hardware/i2c.h"
#include "arm11/hardware/mcu.h"

enum McuRegisters {
	RegBattery = 0x0B,
	RegExHW = 0x0F,
	RegPower = 0x20,
	RegLCDs = 0x22,
	RegWifiLED = 0x2A,
	Reg3DLED = 0x2C,
};

void MCU_disableLEDs(void)
{
	// disable wifi LED
	I2C_writeReg(I2C_DEV_MCU, RegWifiLED, 0);
	
	// disable 3D LED
	I2C_writeReg(I2C_DEV_MCU, Reg3DLED, 0);
}

void MCU_powerOnLCDs(void)
{
	// bit1 = lcd power enable for both screens
	I2C_writeReg(I2C_DEV_MCU, RegLCDs, 1<<5 | 1<<3 | 1<<1);
}

void MCU_powerOffLCDs(void)
{
	// bit0 = lcd power disable for both screens (also disables backlight)
	I2C_writeReg(I2C_DEV_MCU, RegLCDs, 1);
}

void MCU_triggerPowerOff(void)
{
	I2C_writeReg(I2C_DEV_MCU, RegPower, 1);
}

void MCU_triggerReboot(void)
{
	I2C_writeReg(I2C_DEV_MCU, RegPower, 1u << 2);
}

u8 MCU_readBatteryLevel(void)
{
	u8 state;

	if(!I2C_readRegBuf(I2C_DEV_MCU, RegBattery, &state, 1))
		return 0;
	
	return state;
}

bool MCU_readBatteryChargeState(void)
{
	u8 state;
	
	state = I2C_readReg(I2C_DEV_MCU, RegExHW);
	
	return (state & (1u << 4)) == 1;
}
