#pragma once

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


#define I2C_STOP          (1u)
#define I2C_START         (1u<<1)
#define I2C_ERROR         (1u<<2)
#define I2C_ACK           (1u<<4)
#define I2C_DIRE_WRITE    (0u)
#define I2C_DIRE_READ     (1u<<5)
#define I2C_IRQ_ENABLE    (1u<<6)
#define I2C_ENABLE        (1u<<7)

#define I2C_GET_ACK(reg)  ((bool)((reg)>>4 & 1u))


typedef enum
{
	I2C_DEV_POWER     = 0, 	// Unconfirmed
	I2C_DEV_CAMERA    = 1, 	// Unconfirmed
	I2C_DEV_CAMERA2   = 2, 	// Unconfirmed
	I2C_DEV_MCU       = 3,
	I2C_DEV_GYRO      = 10,
	I2C_DEV_DEBUG_PAD = 12,
	I2C_DEV_IR        = 13,
	I2C_DEV_EEPROM    = 14, // Unconfirmed
	I2C_DEV_NFC       = 15,
	I2C_DEV_QTM       = 16,
	I2C_DEV_N3DS_HID  = 17
} I2cDevice;



bool I2C_readRegBuf(I2cDevice devId, u8 regAddr, u8 *out, u32 size);
bool I2C_writeReg(I2cDevice devId, u8 regAddr, u8 data);

u8 i2cmcu_readreg_hid_irq(void);
u8 i2cmcu_readreg_hid_held(void);
bool i2cmcu_lcd_poweron(void);
bool i2cmcu_lcd_backlight_poweron(void);
bool i2cmcu_lcd_poweroff(void);

#define MCU_HID_POWER_BUTTON_PRESSED       (1u)
#define MCU_HID_POWER_BUTTON_LONG_PRESSED  (1u<<1)
#define MCU_HID_HOME_BUTTON_PRESSED        (1u<<2)
#define MCU_HID_HOME_BUTTON_RELEASED       (1u<<3)
#define MCU_HID_HOME_BUTTON_NOT_HELD       (1u<<1)
#define MCU_HID_SHELL_GOT_CLOSED           (1u<<5)
#define MCU_HID_SHELL_GOT_OPENED           (1u<<6)
