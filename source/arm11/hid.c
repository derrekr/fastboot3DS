/*
 *  Original code from ctrulib
 */

#include "types.h"
#include "mem_map.h"
#include "arm11/hid.h"
#include "arm11/i2c.h"
#include "arm11/interrupt.h"
#include "arm11/gpio.h"


#define REG_HID_PAD  (*((vu16*)(IO_MEM_ARM9_ARM11 + 0x46000)) ^ 0xFFFFu)


static u32 kHeld, kDown, kUp, homeShellState, powerWifiState;



static void hidIrqHandler(UNUSED u32 intSource);

void hidInit(void)
{
	IRQ_registerHandler(IRQ_MCU_HID, 14, 0, true, hidIrqHandler);
	GPIO_setBit(19, 9); // This enables the MCU HID IRQ
}

static void hidIrqHandler(UNUSED u32 intSource)
{
	const u32 state = (u32)i2cmcu_readreg_hid_irq();

	powerWifiState |= state & 3;
	powerWifiState |= state>>2 & 4;

	homeShellState |= (state & 4)<<19;
	homeShellState |= (state & 0x20)<<17;

	if(homeShellState & KEY_HOME) homeShellState ^= (state & 8)<<18;
	if(homeShellState & KEY_SHELL) homeShellState ^= (state & 0x40)<<16;
}

u32 hidGetPowerButton(void)
{
	u32 tmp = powerWifiState;
	powerWifiState &= ~3;
	return tmp & 3;
}

u32 hidGetWifiButton(void)
{
	u32 tmp = powerWifiState;
	powerWifiState &= ~4;
	return tmp>>2;
}

void hidScanInput(void)
{
	u32 kOld = kHeld;
	kHeld = homeShellState | REG_HID_PAD;
	kDown = (~kOld) & kHeld;
	kUp = kOld & (~kHeld);
}

u32 hidKeysHeld(void)
{
	return kHeld;
}

u32 hidKeysDown(void)
{
	return kDown;
}

u32 hidKeysUp(void)
{
	return kUp;
}
