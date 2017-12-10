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
#include "arm11/hardware/mcu.h"
#include "hardware/cache.h"
#include "arm11/hardware/interrupt.h"
#include "ipc_handler.h"
#include "hardware/pxi.h"



noreturn void power_off(void)
{
	PXI_sendCmd(IPC_CMD9_PREPARE_POWER, NULL, 0);
	MCU_powerOffLCDs();

	flushDCache();
	__asm__ __volatile__("cpsid aif" : : : "memory");

	MCU_triggerPowerOff();

	while(1) __wfi();
}

noreturn void power_reboot(void)
{
	PXI_sendCmd(IPC_CMD9_PREPARE_POWER, NULL, 0);
	MCU_powerOffLCDs();

	flushDCache();
	__asm__ __volatile__("cpsid aif" : : : "memory");

	MCU_triggerReboot();

	while(1) __wfi();
}
