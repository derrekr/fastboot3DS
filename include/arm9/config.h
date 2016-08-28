#pragma once

enum Keys {
	KBootOption1 = 0,
	KBootOption2,
	KBootOption3,
	KBootOption1Buttons,
	KBootOption2Buttons,
	KBootOption3Buttons,
	KBootMode,
	// ...
	KLast
};

#define numKeys  KLast

void *configCopyText(int key);
const void *configGetData(int key);
const char *configGetKeyText(int key);
