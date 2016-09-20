#include "types.h"
#include "mem_map.h"
#include "util.h"
#include "arm11/i2c.h"
#include "gfx.h"
#include "pxi.h"



extern void disableCaches(void);

void turn_off(void)
{
	//printf("Attempting to turn off...\n");
	i2cmcu_lcd_poweroff();
	i2cmcu_lcd_backlight_poweroff();
	for(;;)
	{
		i2c_writeregdata(3, 0x20, 0xff);
		wait(0x200000);
	}
}

void NAKED firmLaunchStub(void)
{
	// Answer ARM0
	REG_PXI_SYNC11 = 0; // Disable all IRQs
	while(REG_PXI_CNT11 & PXI_SEND_FIFO_FULL);
	REG_PXI_SEND11 = PXI_RPL_OK;

	// Wait for entry address
	while(REG_PXI_CNT11 & PXI_RECV_FIFO_EMPTY);
	void (*entry11)(void) = (void (*)(void))REG_PXI_RECV11;

	// Tell ARM9 we got the entry
	while(REG_PXI_CNT11 & PXI_SEND_FIFO_FULL);
	REG_PXI_SEND11 = PXI_RPL_FIRM_LAUNCH_READY;
	REG_PXI_CNT11 = 0; // Disable PXI

	entry11();
}

int main(void)
{
	// Relocate ARM11 stub
	for(u32 i = 0; i < 0x200>>2; i++)
	{
		((u32*)A11_STUB_ENTRY)[i] = ((u32*)firmLaunchStub)[i];
	}

	PXI_init();
	gfx_init();

	for(;;)
	{
		bool successFlag;
		u32 cmdCode = PXI_tryRecvWord(&successFlag);

		// process cmd
		if(successFlag)
		{

			switch(cmdCode)
			{
				case PXI_CMD_FIRM_LAUNCH:
					goto start_firmlaunch;
				default:
					break;
			}
		}

		/* Update state, check for changes */
		u8 hidstate = i2cmcu_readreg_hid();

		if(hidstate & MCU_HID_POWER_BUTTON_PRESSED)
			turn_off();
		if(hidstate & MCU_HID_SHELL_GOT_CLOSED)
			i2cmcu_lcd_poweroff();
		else if(hidstate & MCU_HID_SHELL_GOT_OPENED)
		{
			i2cmcu_lcd_poweron();
			i2cmcu_lcd_backlight_poweron();
		}
		if(hidstate & MCU_HID_HOME_BUTTON_PRESSED)
			PXI_trySendWord(PXI_RPL_HOME_PRESSED);
		wait(0x8000);
	}

start_firmlaunch:

	// Turn off LCDs and backlight before FIRM launch.
	i2cmcu_lcd_poweroff();
	i2cmcu_lcd_backlight_poweroff();

	void (*stub)(void) = (void (*)(void))A11_STUB_ENTRY;
	disableCaches();
	stub();

	return 0;
}
