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
 


// ANSI escape sequences for colors
// see: https://en.wikipedia.org/wiki/ANSI_escape_code
#define ESC_RESET			"\x1b[0m"
#define ESC_BOLD			"\x1b[1m"
#define ESC_FAINT			"\x1b[2m"
#define ESC_UNDERLINE		"\x1b[4m"
#define ESC_CROSSEDOUT		"\x1b[9m"
#define ESC_INVERT			"\x1b[7m"
#define ESC_FGCOLOR(x)		"\x1b[3" #x "m"
#define ESC_BGCOLOR(x)		"\x1b[4" #x "m"

// colors for color scheme
#define ESC_SCHEME_STD		ESC_RESET ESC_FGCOLOR(7) ESC_BGCOLOR(0)
#define ESC_SCHEME_GOOD		ESC_RESET ESC_FGCOLOR(2) ESC_BGCOLOR(0) ESC_BOLD
#define ESC_SCHEME_WEAK		ESC_RESET ESC_FGCOLOR(7) ESC_BGCOLOR(0) ESC_BOLD
#define ESC_SCHEME_ACCENT0	ESC_RESET ESC_FGCOLOR(4) ESC_BGCOLOR(0) ESC_BOLD
#define ESC_SCHEME_ACCENT1	ESC_RESET ESC_FGCOLOR(3) ESC_BGCOLOR(0) ESC_BOLD
