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
#include "hardware/pxi.h"
#include "arm11/start.h"
#include "arm11/hardware/interrupt.h"
#include "arm11/hardware/timer.h"
#include "arm11/hardware/i2c.h"
#include "arm11/hardware/mcu.h"
#include "arm11/hardware/hid.h"
#include "arm11/hardware/cpu.h"
#include "arm.h"



static void systemRestoreHwState(void)
{
	MCU_disableLEDs();
}

void systemInit(void)
{
	IRQ_init();
	TIMER_init();

	const u32 cpuId = __getCpuId();
	if(!cpuId)
	{
		IRQ_registerHandler(IRQ_PDN, 0, 0b1111, true, NULL);
		I2C_init();
		hidInit();
		PXI_init();
		MCU_init();
		systemRestoreHwState();
	}
	else
	{
		// We don't need core 1 yet so back it goes into boot11.
		// Core 2 and 3 also go there waiting until poweroff.
		deinitCpu();
		((void (*)(void))0x0001004C)();
	}

	__cpsie(i); // Enables interrupts
}
