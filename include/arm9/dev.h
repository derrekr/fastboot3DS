
#pragma once

typedef struct
{
	char *name;
	bool initialized;
	bool (*init)();
	bool (*read)(u32 offset, u32 size, void *buf);
	bool (*write)(u32 offset, u32 size, void *buf);
	bool (*close)();
	bool (*is_active)();
	
} dev_struct;

extern const dev_struct *dev_sdcard;
extern const dev_struct *dev_rawnand;
extern const dev_struct *dev_decnand;
extern const dev_struct *dev_flash;

