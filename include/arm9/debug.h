#pragma once

#include "types.h"

void hashCodeRoData();
noreturn void panic();
void dumpMem(u8 *mem, u32 size, char *filepath);
