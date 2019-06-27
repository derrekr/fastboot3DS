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

#include <string.h>
#include "types.h"
#include "mem_map.h"
#include "arm11/start.h"
#include "hardware/pxi.h"
#include "system.h"
#include "ipc_handler.h"



void NAKED firmLaunchStub(void)
{
	*((vu32*)A11_FALLBACK_ENTRY) = 0;

	// Tell ARM9 we are ready
	REG_PXI_SFIFO = 0xA8E4u;

	// Wait for entry address
	while(REG_PXI_CNT & PXI_CNT_RFIFO_EMPTY);
	u32 entry = REG_PXI_RFIFO;

	// Tell ARM9 we got the entry
	REG_PXI_SFIFO = 0x94C6u;

	if(!entry) while(!(entry = *((vu32*)A11_FALLBACK_ENTRY)));

	((void (*)(void))entry)();
}

s32 loadVerifyFirm(const char *const path, bool skipHashCheck)
{
	u32 cmdBuf[3];
	cmdBuf[0] = (u32)path;
	cmdBuf[1] = strlen(path) + 1;
	cmdBuf[2] = skipHashCheck;

	return PXI_sendCmd(IPC_CMD9_LOAD_VERIFY_FIRM, cmdBuf, 3);
}

noreturn void firmLaunch(void)
{
	PXI_sendCmd(IPC_CMD9_FIRM_LAUNCH, NULL, 0);

	// Relocate ARM11 stub
	memcpy((void*)A11_STUB_ENTRY, (const void*)firmLaunchStub, A11_STUB_SIZE);

	__systemDeinit();
	deinitCpu();

	// Change sp to a safe location
	register vu32 sp __asm__("sp");
	sp = A11_STUB_ENTRY;
	(void)sp; // Shut up unused variable warning.

	((void (*)(void))A11_STUB_ENTRY)();
	while(1);
}
