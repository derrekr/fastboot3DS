/*
 *  Original code from ctrulib
 */

#include "types.h"
#include "hid.h"

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
