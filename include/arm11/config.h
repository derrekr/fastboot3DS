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


enum Keys {
	KBootOption1 = 0,
	KBootOption2,
	KBootOption3,
	KBootOption1NandImage,
	KBootOption2NandImage,
	KBootOption3NandImage,
	KBootOption1Buttons,
	KBootOption2Buttons,
	KBootOption3Buttons,
	KBootMode,
	KDevMode,
	// ...
	KLast
};

enum BootModes {
	BootModeNormal = 0,
	BootModeQuick,
	BootModeQuiet
};

#define numKeys  KLast

bool loadConfigFile();
bool writeConfigFile();
void *configCopyText(int key);
const void *configGetData(int key);
bool configDataExist(int key);
const char *configGetKeyText(int key);
bool configSetKeyData(int key, const void *data);
void configRestoreDefaults();
bool configDeleteKey(int key);
bool configDevModeEnabled();
