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



/**
 * @brief      Sets a bit in the specified GPIO register.
 *
 * @param[in]  reg     The register number.
 * @param[in]  bitNum  The bit number.
 */
void GPIO_setBit(u16 reg, u8 bitNum);

/**
 * @brief      Clears a bit in the specified GPIO register.
 *
 * @param[in]  reg     The register number.
 * @param[in]  bitNum  The bit number.
 */
void GPIO_clearBit(u16 reg, u8 bitNum);
