#pragma once

bool isFirmLoaded(void);
bool menuLaunchFirm(const char *filePath, bool quick);
bool tryLoadFirmwareFromSettings(bool fromMenu);
bool tryLoadFirmware(const char *filepath, bool skipHashCheck, bool printInfo);
