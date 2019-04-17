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
#include "arm11/hardware/mcu.h"
#include "arm11/hardware/interrupt.h"
#include "arm11/hardware/gpio.h"


static u32 kHeld = 0, kDown = 0, kUp = 0;
static u32 homeShellState = 0, powerWifiState = 0;
static volatile bool mcuIrq = false;



static void hidIrqHandler(UNUSED u32 intSource);

void hidInit(void)
{
	// When set after a reboot from TWL_FIRM while holding the HOME button this will
	// cause the MCU to spam IRQs infinitely after releasing the HOME button.
	// mcuBugs++
	// Update: For some reason this is not reproduceable anymore.
	MCU_setIrqBitmask(0xFFFF1800u); // Standard bitmask at cold boot

	IRQ_registerHandler(IRQ_MCU_HID, 14, 0, true, hidIrqHandler);
	GPIO_setBit(19, 9); // This enables the MCU IRQ.
}

static void hidIrqHandler(UNUSED u32 intSource)
{
	mcuIrq = true;
}

static void updateMcuIrqState(void)
{
	// TODO: We should probably disable IRQs temporarily here.
	if(!mcuIrq) return;
	mcuIrq = false;

	const u32 state = MCU_readReceivedIrqs();

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
	// Mask out power button pressed and keep power button long pressed + WiFi button state.
	if(resetState) powerWifiState = tmp & 6;
	return tmp & 3;
}

u32 hidGetWifiButton(bool resetState)
{
	u32 tmp = powerWifiState;
	// Mask out WiFi button and keep power button states.
	if(resetState) powerWifiState = tmp & 3;
	return tmp>>2;
}

bool hidIsHomeButtonHeldRaw(void)
{
	return !(MCU_readHidHeld() & 1u<<1);
}

void hidScanInput(void)
{
	updateMcuIrqState();

	u32 kOld = kHeld;
	kHeld = homeShellState | REG_HID_PAD;;
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
