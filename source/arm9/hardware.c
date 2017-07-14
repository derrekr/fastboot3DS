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
#include "arm9/interrupt.h"
#include "arm9/ndma.h"
#include "arm9/timer.h"
#include "pxi.h"
#include "arm9/crypto.h"



void hardwareInit(void)
{
	IRQ_init();
	NDMA_init();
	TIMER_init();
	PXI_init();
	AES_init();
}

void hardwareDeinit(void)
{
	// New 3DS K9L doesn't like FIFO counts >4 and hangs.
	// Thx Nintendo
	AES_deinit();
}
