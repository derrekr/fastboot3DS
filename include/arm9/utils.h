#pragma once

#include <stdbool.h>
#include "types.h"



bool loadFirmNand(void);
bool loadFile(const char *filePath, void *address, u32 size, u32 *bytesRead);
bool makeWriteFile(const char *filePath, void *address, u32 size, u32 *bytesWritten);
bool updateNandLoader(const char *filePath);
u64 getFreeSpace(const char *drive);
bool dumpNand(void);
bool restoreNand(void);
void initGfx(void);
