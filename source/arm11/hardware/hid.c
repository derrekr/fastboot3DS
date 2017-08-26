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


static u32 kHeld, kDown, kUp, homeShellState, powerWifiState;



static void hidIrqHandler(UNUSED u32 intSource);

void hidInit(void)
{
	IRQ_registerHandler(IRQ_MCU_HID, 14, 0, true, hidIrqHandler);
	GPIO_setBit(19, 9); // This enables the MCU HID IRQ
}

static void hidIrqHandler(UNUSED u32 intSource)
{
	const u32 state = (u32)i2cmcu_readreg_hid_irq();

	powerWifiState |= state & 3;
	powerWifiState |= state>>2 & 4;

	homeShellState |= (state & 4)<<19;
	homeShellState |= (state & 0x20)<<17;

	if(homeShellState & KEY_HOME) homeShellState ^= (state & 8)<<18;
	if(homeShellState & KEY_SHELL) homeShellState ^= (state & 0x40)<<16;
}

u32 hidGetPowerButton(bool resetState)
{
	u32 tmp = powerWifiState;
	if(resetState) powerWifiState &= ~3;
	return tmp & 3;
}

u32 hidGetWifiButton(bool resetState)
{
	u32 tmp = powerWifiState;
	if(resetState) powerWifiState &= ~4;
	return tmp>>2;
}

void hidScanInput(void)
{
	u32 kOld = kHeld;
	kHeld = homeShellState | REG_HID_PAD;
	kDown = (~kOld) & kHeld;
	kUp = kOld & (~kHeld);
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
