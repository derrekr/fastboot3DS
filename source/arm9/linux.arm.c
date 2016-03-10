#include <stdio.h>
#include <stdbool.h>
#include "types.h"
#include "cache.h"
#include "mem_map.h"
#include "arm9/fatfs/ff.h"
#include "arm9/ndma.h"
#include "arm9/utils.h"
#include "arm9/linux_config.h"
#include "arm9/linux.h"



void loadLinux(void)
{
	u32 bytesRead;
	extern u32 linux_payloads_start;
	extern u32 linux_payloads_end;


	initGfx();
	loadFile(LINUXIMAGE_FILENAME, (void*)ZIMAGE_ADDR, 0x4000000, NULL);
	loadFile(DTB_FILENAME, (void*)PARAMS_TMP_ADDR, 0x400000, &bytesRead);
	f_mount(NULL, "sdmc:", 1);

	*((vu32*)0x214FFFFC) = bytesRead;

	NDMA_copy((void*)0x23F00000, &linux_payloads_start, ((u32)&linux_payloads_end - (u32)&linux_payloads_start)>>2);
	invalidateICacheRange((void*)0x23F00000, ((u32)&linux_payloads_end - (u32)&linux_payloads_start));

	*((vu32*)CORE_SYNC_ID) = 0x544F4F42;

	__asm__ __volatile__("ldr pc, =0x23F00000");
}
