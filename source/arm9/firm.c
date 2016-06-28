#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "arm9/firm.h"
#include "mem_map.h"
#include "arm9/crypto.h"
#include "cache.h"
#include "arm9/ndma.h"
#include "arm9/mpu.h"



void NORETURN firmLaunchStub(void)
{	
	firm_header *firm_hdr = (firm_header*)FIRM_LOAD_ADDR;
	void (*entry9)(void) = firmLaunchEntry9;
	if(firmLaunchEntry9 == NULL) entry9 = (void (*)(void))firm_hdr->entrypointarm9;
	void (*entry11)(void) = firmLaunchEntry11;
	if(firmLaunchEntry11 == NULL) entry11 = (void (*)(void))firm_hdr->entrypointarm11;


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
	*((vu32*)CORE_SYNC_PARAM) = (u32)entry11;

	// Wait for ARM11...
	while(*((vu32*)CORE_SYNC_ID) != 0x544F4F42);

	// go for it!
	__asm__ __volatile__("mov lr, %[in]\n" : : [in] "r" (firm_hdr->entrypointarm9));
	entry9();
	while(1);
}

bool firm_load_verify(void)
{
	firm_header *firm_hdr = (firm_header*)FIRM_LOAD_ADDR;
	const char *res[2] = {"\x1B[31mBAD", "\x1B[32mGOOD"};
	bool isValid;
	bool retval = true;
	u32 hash[8];

	printf("ARM9  entry: 0x%"PRIX32"\n", firm_hdr->entrypointarm9);
	printf("ARM11 entry: 0x%"PRIX32"\n", firm_hdr->entrypointarm11);

	for(int i=0; i<4; i++)
	{
		firm_sectionheader *section = &firm_hdr->section[i];

		if(section->size == 0)
			continue;

		printf("Section %i:\noffset: 0x%"PRIX32", addr: 0x%"PRIX32", size 0x%"PRIX32"\n",
				i, section->offset, section->address, section->size);

		sha((u32*)(FIRM_LOAD_ADDR + section->offset), section->size, hash, SHA_INPUT_BIG | SHA_MODE_256, SHA_OUTPUT_BIG);
		isValid = memcmp(hash, section->hash, 32) == 0;
		printf("Hash: %s\e[0m\n", res[isValid]);

		retval &= isValid;
	}
	
	return retval;
}

void NORETURN firm_launch(void)
{
	*((vu32*)CORE_SYNC_ID) = 0x544F4F42;

	//printf("Waiting for ARM11...\n");
	while(*((vu32*)CORE_SYNC_ID) != 0x4F4B4F4B);
	
	//printf("Relocating FIRM launch stub...\n");
	flushDCacheRange((void*)A9_STUB_ENTRY, 0x200);
	NDMA_copy((u32*)A9_STUB_ENTRY, (u32*)firmLaunchStub, 0x200>>2);

	//printf("Starting firm launch...\n");
	void (*stub)(void) = (void (*)(void))A9_STUB_ENTRY;
	disableMpu();
	stub();
	while(1);
}
