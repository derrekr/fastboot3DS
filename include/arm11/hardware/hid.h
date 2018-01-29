#pragma once

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

/*
 *  Based on code from https://github.com/smealum/ctrulib
 */

#include "types.h"


#define REG_HID_PAD  (*((vu16*)(IO_MEM_ARM9_ARM11 + 0x46000)) ^ 0xFFFFu)


#define HID_KEY_MASK_ALL          ((KEY_SHELL << 1) - 1)
#define HID_VERBOSE_MODE_BUTTONS  (KEY_SELECT | KEY_START)

enum
{
	KEY_A       = (1),
	KEY_B       = (1<<1),
	KEY_SELECT  = (1<<2),
	KEY_START   = (1<<3),
	KEY_DRIGHT  = (1<<4),
	KEY_DLEFT   = (1<<5),
	KEY_DUP     = (1<<6),
	KEY_DDOWN   = (1<<7),
	KEY_R       = (1<<8),
	KEY_L       = (1<<9),
	KEY_X       = (1<<10),
	KEY_Y       = (1<<11),
	KEY_HOME    = (1<<21),
	KEY_SHELL   = (1<<22)
};



#ifdef __cplusplus
extern "C"
{
#endif

void hidInit(void);
u32 hidGetPowerButton(bool resetState);
u32 hidGetWifiButton(bool resetState);
bool hidIsHomeButtonHeldRaw(void);
void hidScanInput(void);
u32 hidKeysHeld(void);
u32 hidKeysDown(void);
u32 hidKeysUp(void);

#ifdef __cplusplus
}
#endif
