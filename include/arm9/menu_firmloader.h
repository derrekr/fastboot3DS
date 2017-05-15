#pragma once

bool isFirmLoaded(void);
bool menuLaunchFirm(const char *filePath, bool quick);
bool menuTryLoadFirmwareFromSettings(void);
bool TryLoadFirmwareFromSettings(void);
bool tryLoadFirmware(const char *filepath, bool skipHashCheck, bool printInfo);
