/*
 *  Original code from ctrulib
 */

#include "mem_map.h"
#include "types.h"
#include "hid.h"


#define HID_REGS_BASE  (IO_MEM_ARM9_ARM11 + 0x00046000)
#define REG_HID_PAD    ~(*((vu32*)(HID_REGS_BASE)))


static u32 kOld, kHeld, kDown, kUp;



void hidScanInput(void)
{
	kOld = kHeld;
	kHeld = REG_HID_PAD;
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
