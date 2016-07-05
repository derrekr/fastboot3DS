#pragma once

#include "types.h"



typedef struct
{
	const char *name;
	bool initialized;
	bool (*init)();
	bool (*read_sector)(u32 sector, u32 count, void *buf);
	bool (*write_sector)(u32 sector, u32 count, const void *buf);
	bool (*close)();
	bool (*is_active)();
	u32  (*get_sector_count)();
} dev_struct;

extern const dev_struct *dev_sdcard;
extern const dev_struct *dev_rawnand;
extern const dev_struct *dev_decnand;
extern const dev_struct *dev_flash;
