#pragma once

#include "types.h"

#define NANDIMG_ERROR_BADPATH	-1
#define NANDIMG_ERROR_NEXISTS	-2
#define NANDIMG_ERROR_NCONTS	-3

int validateNandImage(const char *filePath);
