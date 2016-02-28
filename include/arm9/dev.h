#pragma once

#include "types.h"
#include <stdbool.h>



typedef struct
{
	char *name;
	bool initialized;
	bool (*init)();
	bool (*read)(u32 sector_offset, u32 sector_count, void *buf);
	bool (*write)(u32 sector_offset, u32 sector_count, void *buf);
	bool (*close)();
	bool (*is_active)();
	
} dev_struct;

extern const dev_struct *dev_sdcard;

