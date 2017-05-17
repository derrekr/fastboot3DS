#pragma once

#include "types.h"

void debugHashCodeRoData();
noreturn void panic();
noreturn void panicMsg(const char *msg);
void dumpMem(u8 *mem, u32 size, char *filepath);
