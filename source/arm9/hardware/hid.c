/*
 * This code is part of ctrulib (https://github.com/smealum/ctrulib)
 */

#include "mem_map.h"
#include "types.h"


#define REG_HID_PAD  (*((vu16*)(IO_MEM_ARM9_ARM11 + 0x46000)) ^ 0xFFFFu)


static u32 kHeld, kDown, kUp;



void hidScanInput(void)
{
	u32 kOld = kHeld;
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
