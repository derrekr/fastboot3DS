/*
 *   This file is part of fastboot 3DS
 *   Copyright (C) 2019 Sergi Granell (xerpi), Paul LaMendola (paulguy), derrek, profi200
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

// Based on code from https://github.com/xerpi/linux_3ds/blob/master/drivers/input/misc/nintendo3ds_codec_hid.c

#include "types.h"
#include "arm11/hardware/spi.h"
#include "arm11/hardware/timer.h"
#include "arm11/hardware/gpio.h"


typedef struct
{
	u8 driverGainHP;
	u8 driverGainSP;
	u8 analogVolumeHP;
	u8 analogVolumeSP;
	s8 shutterVolume[2];
	u8 microphoneBias;
	u8 quickCharge;
	u8 PGA_GAIN; // microphone gain
	u8 reserved[3];
	s16 filterHP32[15]; // 3 * 5
	s16 filterHP47[15];
	s16 filterSP32[15];
	s16 filterSP47[15];
	s16 filterMic32[28]; // (1+2)+((1+4)*5)
	s16 filterMic47[28];
	s16 filterFree[28];
	u8 analogInterval;
	u8 analogStabilize;
	u8 analogPrecharge;
	u8 analogSense;
	u8 analogDebounce;
	u8 analog_XP_Pullup;
	u8 YM_Driver;
	u8 reserved2;
} CodecCal;


alignas(4) static CodecCal fallbackCal =
{
	0u,
	1u,
	0u,
	7u,
	{0xFD, 0xEC},
	3u,
	2u,
	0u,
	{0, 0, 0},
	{32767, 0, 0, 0, 0, 32767, 0, 0, 0, 0, 32736, 49168, 0, 16352, 0},
	{32767, 0, 0, 0, 0, 32767, 0, 0, 0, 0, 32745, 49164, 0, 16361, 0},
	{32767, 38001, 22413, 30870, 36440, 51536, 30000, 51536, 0, 0, 32736, 49168, 0, 16352, 0},
	{32767, 36541, 25277, 31456, 35336, 51134, 30000, 51134, 0, 0, 32745, 49164, 0, 16361, 0},
	{32767, 0, 0, 32767, 0, 0, 0, 0, 32767, 0, 0, 0, 0, 32767, 0, 0, 0, 0, 32767, 0, 0, 0, 0, 32767, 0, 0, 0, 0},
	{32767, 0, 0, 32767, 0, 0, 0, 0, 32767, 0, 0, 0, 0, 32767, 0, 0, 0, 0, 32767, 0, 0, 0, 0, 32767, 0, 0, 0, 0},
	{32767, 0, 0, 52577, 56751, 32767, 8785, 12959, 52577, 56751, 32767, 8785, 12959, 52577, 56751, 32767, 8785, 12959, 32767, 0, 0, 0, 0, 32767, 0, 0, 0, 0},
	1u,
	9u,
	4u,
	3u,
	0u,
	6u,
	1u,
	0u
};



static void codecSwitchBank(u8 bank)
{
	static u8 curBank = 0x63;
	if(bank != curBank)
	{
		alignas(4) u8 inBuf[4];
		inBuf[0] = 0; // Write
		inBuf[1] = bank;
		NSPI_writeRead(NSPI_DEV_CTR_CODEC, (u32*)inBuf, NULL, 2, 0, true);

		curBank = bank;
	}
}

static void codecReadRegBuf(u8 bank, u8 reg, u32 *buf, u32 size)
{
	codecSwitchBank(bank);

	alignas(4) u8 inBuf[4];
	inBuf[0] = reg<<1 | 1u;
	NSPI_writeRead(NSPI_DEV_CTR_CODEC, (u32*)inBuf, buf, 1, size, true);
}

static u8 codecReadReg(u8 bank, u8 reg)
{
	alignas(4) u8 outBuf[4];
	codecReadRegBuf(bank, reg, (u32*)outBuf, 1);

	return outBuf[0];
}

static void codecWriteRegBuf(u8 bank, u8 reg, u32 *buf, u32 size)
{
	codecSwitchBank(bank);

	alignas(4) u8 inBuf[4];
	inBuf[0] = reg<<1; // Write
	NSPI_writeRead(NSPI_DEV_CTR_CODEC, (u32*)inBuf, NULL, 1, 0, false);
	NSPI_writeRead(NSPI_DEV_CTR_CODEC, buf, NULL, size, 0, true);
}

static void codecWriteReg(u8 bank, u8 reg, u8 val)
{
	codecSwitchBank(bank);

	alignas(4) u8 inBuf[4];
	inBuf[0] = reg<<1; // Write
	inBuf[1] = val;
	NSPI_writeRead(NSPI_DEV_CTR_CODEC, (u32*)inBuf, NULL, 2, 0, true);
}

static void codecMaskReg(u8 bank, u8 reg, u8 val, u8 mask)
{
	u8 data = codecReadReg(bank, reg);
	data = (data & ~mask) | (val & mask);
	codecWriteReg(bank, reg, data);
}

// Helpers
static void codecSwapCalibrationData(CodecCal *cal)
{
	u16 *tmp = (u16*)cal->filterHP32;
	for(int i = 0; i < 15; i++)
	{
		tmp[i] = __builtin_bswap16(tmp[i]);
	}

	tmp = (u16*)cal->filterHP47;
	for(int i = 0; i < 15; i++)
	{
		tmp[i] = __builtin_bswap16(tmp[i]);
	}

	tmp = (u16*)cal->filterSP32;
	for(int i = 0; i < 15; i++)
	{
		tmp[i] = __builtin_bswap16(tmp[i]);
	}

	tmp = (u16*)cal->filterSP47;
	for(int i = 0; i < 15; i++)
	{
		tmp[i] = __builtin_bswap16(tmp[i]);
	}

	tmp = (u16*)cal->filterMic32;
	for(int i = 0; i < 28; i++)
	{
		tmp[i] = __builtin_bswap16(tmp[i]);
	}

	tmp = (u16*)cal->filterMic47;
	for(int i = 0; i < 28; i++)
	{
		tmp[i] = __builtin_bswap16(tmp[i]);
	}

	tmp = (u16*)cal->filterFree;
	for(int i = 0; i < 28; i++)
	{
		tmp[i] = __builtin_bswap16(tmp[i]);
	}
}

static void codecMaskWaitReg(u8 bank, u8 reg, u8 val, u8 mask)
{
	for(u32 i = 0; i < 64; i++) // Some kind of timeout? No error checking.
	{
		codecMaskReg(bank, reg, val, mask);
		if((codecReadReg(bank, reg) & mask) == val) break;
	}
}

static void codecEnableTouchscreen(void)
{
	codecMaskReg(0x67, 0x26, 0x80, 0x80);
	codecMaskReg(0x67, 0x24, 0, 0x80);
	codecMaskReg(0x67, 0x25, 0x10, 0x3C);
}

static void codecDisableTouchscreen(void)
{
	codecMaskReg(0x67, 0x26, 0, 0x80);
	codecMaskReg(0x67, 0x24, 0x80, 0x80);
}


void CODEC_init(void)
{
	static bool inited = false;
	if(inited) return;
	inited = true;


	NSPI_init();

	// TODO: Load calibration from HWCAL files on eMMC.
	CodecCal *const cal = &fallbackCal;
	codecSwapCalibrationData(cal); // Come the fuck on. Why is this not stored in the correct endianness?

	// Circle pad
	codecWriteReg(0x67, 0x24, 0x98);
	codecWriteReg(0x67, 0x26, 0x00);
	codecWriteReg(0x67, 0x25, 0x43);
	codecWriteReg(0x67, 0x24, 0x18);
	codecWriteReg(0x67, 0x17, cal->analogPrecharge<<4 | cal->analogSense);
	codecWriteReg(0x67, 0x19, cal->analog_XP_Pullup<<4 | cal->analogStabilize);
	codecWriteReg(0x67, 0x1B, cal->YM_Driver<<7 | cal->analogDebounce);
	codecWriteReg(0x67, 0x27, 0x10u | cal->analogInterval);
	codecWriteReg(0x67, 0x26, 0xEC);
	codecWriteReg(0x67, 0x24, 0x18);
	codecWriteReg(0x67, 0x25, 0x53);

	codecEnableTouchscreen();
}

void CODEC_getRawAdcData(u32 buf[13])
{
	//codecSwitchBank(0x67);
	// This reg read seems useless and doesn't affect funtionality.
	//codecReadReg(0x26);

	codecReadRegBuf(0xFB, 1, buf, 52);
}
