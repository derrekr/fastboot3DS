#pragma once

#include "arm9/firm.h"

/* in sectors */
#define FIRMWRITER_SECTORS_PER_BLOCK	(0x1000 / 0x200)

bool firmwriterInit(size_t sector, size_t blockCount, bool preserveSignature);
size_t firmwriterWriteBlock();
bool firmwriterIsDone();
size_t firmwriterFinish();
