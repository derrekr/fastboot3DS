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
 
#include "types.h"
#include "arm11/hardware/hid.h"
#include "arm11/console.h"
#include "arm11/fmt.h"

u32 DummyFunc(u32 param)
{
	// init top console
	PrintConsole top_con;
	consoleInit(SCREEN_TOP, &top_con, true);
	
	// print something
	ee_printf("Hello! I'm a dummy function.\nparam was %lu\n\nPress B to quit.", param);
	
	// wait for A button
	do {
		hidScanInput();
	}
	while (!(hidKeysHeld() & KEY_B));
	
	return 0;
}
