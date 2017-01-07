#pragma once

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
