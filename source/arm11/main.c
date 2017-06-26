#include "types.h"
#include "util.h"
#include "arm11/hardware.h"
#include "arm11/i2c.h"
#include "gfx.h"
#include "pxi.h"
#include "arm11/interrupt.h"
#include "arm11/gpio.h"
#include "arm11/firm.h"
#include "arm11/hid.h"



extern bool battery_ok(void);
extern void power_off(void);
extern void power_reboot(void);

int main(void)
{
	bool poweroff_allowed = false;

	hardwareInit();

	IRQ_registerHandler(IRQ_PXI_SYNC, 14, 0, true, NULL);

	for(;;)
	{
		waitForInterrupt();

		bool successFlag;
		u32 cmdCode = PXI_tryRecvWord(&successFlag);

		// process cmd
		if(successFlag)
		{

			switch(cmdCode)
			{
				case PXI_CMD_ENABLE_LCDS:
					gfx_init();
					PXI_sendWord(PXI_RPL_OK);
					break;
				case PXI_CMD_ALLOW_POWER_OFF:
					poweroff_allowed = true;
					break;
				case PXI_CMD_FORBID_POWER_OFF:
					poweroff_allowed = false;
					break;
				case PXI_CMD_POWER_OFF:
					power_off();
					break;
				case PXI_CMD_REBOOT:
					power_reboot();
					break;
				case PXI_CMD_FIRM_LAUNCH:
					goto start_firmlaunch;
				default:
					break;
			}
		}


		hidScanInput();
		const u32 kDown = hidKeysDown();
		const u32 kUp = hidKeysUp();

		if(hidGetPowerButton())
		{
			if(poweroff_allowed) power_off();
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

start_firmlaunch:
	gfx_deinit(); // TODO: Let ARM9 decide when to deinit gfx

	firm_launch();

	return 0;
}
