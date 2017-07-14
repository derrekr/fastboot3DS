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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "mem_map.h"
#include "arm9/console.h"
#include "arm9/dev.h"
#include "fatfs/ff.h"
#include "pxi.h"
#include "arm9/hid.h"
#include "util.h"
#include "arm9/main.h"
#include "arm9/timer.h"
#include "arm9/menu.h"
#include "arm9/ui.h"

void menuCredits(void)
{
	u32 keys;
	unsigned i = 3;
	
	uiClearConsoles();

	uiPrintCenteredInLine(1, "Credits");

	uiPrintCenteredInLine(i++, "Main developers:");
	uiPrintCenteredInLine(i++, "derrek");
	uiPrintCenteredInLine(i++, "profi200");
	i++;
	uiPrintCenteredInLine(i++, "Thanks to:");
	uiPrintCenteredInLine(i++, "yellows8");
	uiPrintCenteredInLine(i++, "plutoo");
	uiPrintCenteredInLine(i++, "smea");
	uiPrintCenteredInLine(i++, "Normmatt (for sdmmc code)");
	uiPrintCenteredInLine(i++, "WinterMute (for console code)");
	uiPrintCenteredInLine(i+=2, "... everyone who contriubted to 3dbrew.org");

	for(;;)
	{
		/* Handle HID */
		hidScanInput();
		keys = hidKeysDown();

		if(keys & KEY_B)
		{
			goto done;
		}
		
		switch(menuUpdateGlobalState())
		{
			case MENU_EVENT_HOME_PRESSED:
			case MENU_EVENT_POWER_PRESSED:
			case MENU_EVENT_STATE_CHANGE:
				goto done;
			default:
				break;
		}

		menuActState();
	}

done:

	menuActState();
	menuSetReturnToState(STATE_PREVIOUS);
}
