#include "types.h"
#include "arm11/hardware.h"
#include "arm11/interrupt.h"
#include "arm11/hid.h"
#include "arm11/power.h"
#include "arm11/i2c.h"
#include "pxi.h"
#include "gfx.h"
#include "arm11/firm.h"


bool g_poweroffAllowed = false;
bool g_startFirmLaunch = false;



int main(void)
{
	hardwareInit();

	IRQ_registerHandler(IRQ_PDC0, 14, 0, true, NULL);

	while(1)
	{
		waitForInterrupt();
		GX_textureCopy((u64*)FRAMEBUF_TOP_A_1, 0, (u64*)FRAMEBUF_TOP_A_2, 0, SCREEN_SIZE_TOP + SCREEN_SIZE_SUB, 0);

		if(g_startFirmLaunch) break;

		hidScanInput();
		const u32 kDown = hidKeysDown();
		const u32 kUp = hidKeysUp();

		if(hidGetPowerButton())
		{
			if(g_poweroffAllowed) power_off();
			PXI_trySendWord(PXI_RPL_POWER_PRESSED);
		}

		if(kDown & KEY_HOME) PXI_trySendWord(PXI_RPL_HOME_PRESSED);
		if(kDown & KEY_SHELL) i2cmcu_lcd_poweroff();
		if(kUp & KEY_SHELL)
		{
			i2cmcu_lcd_poweron();
			i2cmcu_lcd_backlight_poweron();
		}


		u8 hidstate = i2cmcu_readreg_hid_held();
		
		if(!(hidstate & MCU_HID_HOME_BUTTON_NOT_HELD))
			PXI_trySendWord(PXI_RPL_HOME_HELD);
	}

	gfx_deinit(); // TODO: Let ARM9 decide when to deinit gfx

	firm_launch();

	return 0;
}
