#pragma once

#include "arm9/firm.h"

void firmwriterInit(size_t sector, size_t sectorCount, bool preserveSignature);
bool firmwriterWriteBlock();
bool firmwriterIsDone();
bool firmwriterFinish();
