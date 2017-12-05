/*
 *   This file is part of fastboot 3DS
 *   Copyright (C) 2017 derrek, profi200, d0k3
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

#include "arm11/bootenv.h"
#include "arm11/hardware/i2c.h"
#include "arm11/menu/bootslot_store.h"

#define BOOTSLOT_STORE_REG	((u8) 0x1E)


static u8 stored_slot = INVALID_BOOT_SLOT;


u8 readStoredBootslot(void)
{
	// stored slot is always zero on cold boots
	if (getBootEnv() == BOOTENV_COLD_BOOT)
		return 0;
	
	if (stored_slot == INVALID_BOOT_SLOT)
		stored_slot = I2C_readReg(I2C_DEV_MCU, BOOTSLOT_STORE_REG);
	
	return stored_slot;
}

bool storeBootslot(u8 slot)
{
	if (slot == INVALID_BOOT_SLOT) // 0xFF is not allowed
		slot = 0; 
	
	if (slot != stored_slot)
	{
		stored_slot = slot;
		return I2C_writeReg(I2C_DEV_MCU, BOOTSLOT_STORE_REG, (u8) slot);
	}
	else
	{
		return true;
	}
}
