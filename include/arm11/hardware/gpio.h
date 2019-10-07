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


#define GPIO_INPUT         (0u)
#define GPIO_OUTPUT        (1u)
#define GPIO_EDGE_FALLING  (0u)
#define GPIO_EDGE_RISING   (1u<<1)
#define GPIO_IRQ_ENABLE    (1u<<2)


typedef enum
{
	GPIO_1_0           =  0u<<4 | 0u,
	GPIO_1_1           =  1u<<4 | 0u,
	GPIO_1_2           =  2u<<4 | 0u,

	GPIO_2_0           =  0u<<4 | 1u,
	GPIO_2_1           =  1u<<4 | 1u,

	GPIO_3_0           =  0u<<4 | 2u,
	GPIO_3_1           =  1u<<4 | 2u,
	GPIO_3_2           =  2u<<4 | 2u,
	GPIO_3_3           =  3u<<4 | 2u,
	GPIO_3_4           =  4u<<4 | 2u,
	GPIO_3_5           =  5u<<4 | 2u,
	GPIO_3_6           =  6u<<4 | 2u,
	GPIO_3_7           =  7u<<4 | 2u,
	GPIO_3_8           =  8u<<4 | 2u,
	GPIO_3_9           =  9u<<4 | 2u,
	GPIO_3_10          = 10u<<4 | 2u,
	GPIO_3_11          = 11u<<4 | 2u,

	GPIO_4_0           =  0u<<4 | 3u,

	// Aliases
	GPIO_1_TOUCHSCREEN = GPIO_1_1, // Unset while the touchscreen is used
	GPIO_1_SHELL       = GPIO_1_2, // 1 when closed

	GPIO_3_HEADPH_JACK = GPIO_3_8, // Unset while headphones are plugged in
	GPIO_3_MCU         = GPIO_3_9
} Gpio;



/**
 * @brief      Configures the specified GPIO.
 *
 * @param[in]  gpio  The gpio.
 * @param[in]  cfg   The configuration.
 */
void GPIO_config(Gpio gpio, u8 cfg);
