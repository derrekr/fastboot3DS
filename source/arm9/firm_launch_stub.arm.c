#include <stdio.h>
#include "types.h"
#include "mem_map.h"
#include "arm9/firm_launch_stub.h"
#include "arm9/firm.h"
#include "arm9/ndma.h"



void firmLaunchStub(void *entry)
{	
	firm_header *firm_hdr = (firm_header*)FIRM_LOAD_ADDR;
	if(entry == NULL) entry = (void*)firm_hdr->entrypointarm9;


	for(int i = 0; i < 4; i++)
	{
		firm_sectionheader *section = &firm_hdr->section[i];
		if(section->size == 0)
			continue;

		REG_NDMA_SRC_ADDR(i) = FIRM_LOAD_ADDR + section->offset;
		REG_NDMA_DST_ADDR(i) = section->address;
		REG_NDMA_WRITE_CNT(i) = section->size>>2;
		REG_NDMA_BLOCK_CNT(i) = NDMA_BLOCK_SYS_FREQ;
		REG_NDMA_CNT(i) = NDMA_DST_UPDATE_INC | NDMA_SRC_UPDATE_INC | NDMA_IMMEDIATE_MODE | NDMA_ENABLE;
	}

	while(REG_NDMA0_CNT & NDMA_ENABLE || REG_NDMA1_CNT & NDMA_ENABLE || REG_NDMA2_CNT & NDMA_ENABLE || REG_NDMA3_CNT & NDMA_ENABLE);

	// Tell ARM11 its entrypoint
	*((vu32*)CORE_SYNC_PARAM) = firm_hdr->entrypointarm11;

	// Wait for ARM11...
	while(*((vu32*)CORE_SYNC_ID) != 0x544F4F42);

	// go for it!
	__asm__ __volatile__("mov lr, %[in]\nbx %[in2]\n" : : [in] "r" (firm_hdr->entrypointarm9), [in2] "r" (entry));
}
