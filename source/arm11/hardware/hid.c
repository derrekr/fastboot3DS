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

#include "types.h"
#include "mem_map.h"
#include "arm11/hardware/hid.h"
#include "arm11/hardware/i2c.h"
#include "arm11/hardware/interrupt.h"
#include "arm11/hardware/gpio.h"


#define REG_HID_PAD  (*((vu16*)(IO_MEM_ARM9_ARM11 + 0x46000)) ^ 0xFFFFu)


static vu32 kHeld = 0, kDown = 0, kUp = 0, homeShellState = 0, powerWifiState = 0;



static void hidIrqHandler(UNUSED u32 intSource);

void hidInit(void)
{
	const u32 mcuInterruptMask = 0xFFFF1800u; // Standard bitmask at cold boot
	I2C_writeRegBuf(I2C_DEV_MCU, 0x18, (const u8*)&mcuInterruptMask, 4);

	IRQ_registerHandler(IRQ_MCU_HID, 14, 0, true, hidIrqHandler);
	GPIO_setBit(19, 9); // This enables the MCU HID IRQ
}

static void hidIrqHandler(UNUSED u32 intSource)
{
	const u32 state = (u32)i2cmcu_readreg_hid_irq();

	u32 tmp = powerWifiState;
	tmp |= state & 3;
	tmp |= state>>2 & 4;
	powerWifiState = tmp;

	tmp = homeShellState;
	tmp |= (state & 4)<<19;
	tmp |= (state & 0x20)<<17;

	if(tmp & KEY_HOME) tmp ^= (state & 8)<<18;
	if(tmp & KEY_SHELL) tmp ^= (state & 0x40)<<16;
	homeShellState = tmp;
}

u32 hidGetPowerButton(bool resetState)
{
	u32 tmp = powerWifiState;
	// Mask out power button pressed and long pressed and keep WiFi button state.
	if(resetState) powerWifiState &= 4;
	return tmp & 3;
}

u32 hidGetWifiButton(bool resetState)
{
	u32 tmp = powerWifiState;
	// Mask out WiFi button and keep power button states.
	if(resetState) powerWifiState &= 3;
	return tmp>>2;
}

bool hidIsHomeButtonHeldRaw(void)
{
	u8 buf[19];
	if(!I2C_readRegBuf(I2C_DEV_MCU, 0x7F, buf, 19)) return false;
	return !(buf[18] & 1u<<1);
}

void hidScanInput(void)
{
	u32 kOld = kHeld;
	u32 tmpKHeld = homeShellState | REG_HID_PAD;
	kHeld = tmpKHeld;
	kDown = (~kOld) & tmpKHeld;
	kUp = kOld & (~tmpKHeld);
}

u32 hidKeysHeld(void)
{
	return kHeld;
}

u32 hidKeysDown(void)
{
	return kDown;
}

u32 hidKeysUp(void)
{
	return kUp;
}
