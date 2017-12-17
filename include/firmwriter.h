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

#include "types.h"


#define FIRMWRITER_BLK_SIZE (0x20000) // 128 KB blocks


enum
{
	UPDATE_ERR_INVALID_FIRM  = -2, // Corrupted FIRM
	UPDATE_ERR_INVALID_SIG   = -3, // Signature verification error
	UPDATE_ERR_NOT_INSTALLED = -9, // fastboot3DS is not installed in firm0:/
	UPDATE_ERR_DOWNGRADE     = -10 // Update file is a lower version than installed
};



s32 writeFirmPartition(const char *const part, bool replaceSig);
s32 loadVerifyUpdate(const char *const path, u32 *const version);
