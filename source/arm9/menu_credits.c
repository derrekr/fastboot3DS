#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "mem_map.h"
#include "arm9/console.h"
#include "arm9/dev.h"
#include "fatfs/ff.h"
#include "pxi.h"
#include "hid.h"
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
