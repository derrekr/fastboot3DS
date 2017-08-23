#pragma once

/*
 *   This file is part of fastboot 3DS
 *   Copyright (C) 2017 derrek, profi200, d0k3
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
#include "menu.h"
#include "menu_func.h"

MenuInfo menu_fb3ds[] =
{
	{
		"FastBoot3DS Main Menu", 4,
		{
			{ "Continue Boot...",			"Dummy description (0)",	&DummyFunc,				0 },
			{ "Enter Submenu",				"Dummy description (1)",	NULL,					1 },
			{ "Dummy Entry (A)",			"Dummy description (A)",	&DummyFunc,				1 },
			{ "Dummy Entry (B)",			"Dummy description (B)",	&DummyFunc,				2 }
		}
	},
	{
		"FastBoot3DS SubMenu (1)", 3,
		{
			{ "Dummy Entry (C)",			"Dummy description (C)",	&DummyFunc,				3 },
			{ "Dummy Entry (D)",			"Dummy description (D)",	&DummyFunc,				4 },
			{ "Enter Next Submenu...",		"Dummy description (2)",	NULL,					2 }
		}
	},
	{
		"FastBoot3DS SubMenu (2)", 1,
		{
			{ "Dummy Entry (E)",			"Dummy description (E)",	&DummyFunc,				5 }
		}
	}
};
