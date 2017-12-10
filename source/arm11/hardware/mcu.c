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

u8 MCU_readBatteryLevel(void)
{
	u8 state;

	if(!I2C_readRegBuf(I2C_DEV_MCU, RegBattery, &state, 1))
		return 0;
	
	return state;
}
