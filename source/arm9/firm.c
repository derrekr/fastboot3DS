#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "types.h"
#include "firm.h"
#include "mem_map.h"
#include "arm9/crypto.h"
#include "arm9/ndma.h"



bool firm_load_verify(void)
{
	firm_header *firm_hdr = (firm_header*)FIRM_LOAD_ADDR;
	const char *res[2] = {"BAD", "GOOD"};
	bool isValid;
	u8 hash[0x20];


	printf("ARM9  entry: 0x%X\n", (unsigned int)firm_hdr->entrypointarm9);
	printf("ARM11 entry: 0x%X\n", (unsigned int)firm_hdr->entrypointarm11);

	for(int i=0; i<4; i++)
	{
		firm_sectionheader *section = &firm_hdr->section[i];

		if(section->size == 0)
			continue;

		printf("Section %i:\noffset: 0x%X, addr: 0x%X, size 0x%X\n", i,
				(unsigned int)section->offset, (unsigned int)section->address, (unsigned int)section->size);

		sha((void*)(FIRM_LOAD_ADDR + section->offset), section->size, hash, SHA_INPUT_BIG | SHA_MODE_256, SHA_OUTPUT_BIG);
		isValid = memcmp(hash, section->hash, 32) == 0;
		printf("Hash: %s\n", res[isValid]);

		if(!isValid) return isValid;
	}
	
	return true;
}

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
	A11_ENTRY_VAL = firm_hdr->entrypointarm11;

	// Wait for ARM11...
	while(CORE_SYNC_VAL != 0x544F4F42);

	// go for it!
	__asm__ __volatile__("mov lr, %[in]\nbx %[in2]" : : [in] "r" (firm_hdr->entrypointarm9), [in2] "r" (entry));
}

void firm_launch(void *entry)
{
	CORE_SYNC_VAL = 0x544F4F42;

	//printf("Waiting for ARM11...\n");
	while(CORE_SYNC_VAL != 0x4F4B4F4B);
	
	//printf("Relocating FIRM launch stub...\n");
	void (*stub_entry)(void *entry) = (void*)A9_STUB_ENTRY;
	NDMA_copy((void*)A9_STUB_ENTRY, firmLaunchStub, 0x200>>2);

	//printf("Starting firm launch...\n");
	stub_entry(entry);
}
