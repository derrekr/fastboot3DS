#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "types.h"
#include "arm9/firm.h"
#include "mem_map.h"
#include "arm9/crypto.h"
#include "arm9/ndma.h"
#include "arm9/mpu.h"
#include "arm9/firm_launch_stub.h"



bool firm_load_verify(void)
{
	firm_header *firm_hdr = (firm_header*)FIRM_LOAD_ADDR;
	const char *res[2] = {"\x1B[31mBAD", "\x1B[32mGOOD"};
	bool isValid;
	bool retval = true;
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
		printf("Hash: %s\e[0m\n", res[isValid]);

		retval &= isValid;
	}
	
	return retval;
}

void firm_launch(void *entry)
{
	*((vu32*)CORE_SYNC_ID) = 0x544F4F42;

	//printf("Waiting for ARM11...\n");
	while(*((vu32*)CORE_SYNC_ID) != 0x4F4B4F4B);
	
	//printf("Relocating FIRM launch stub...\n");
	NDMA_copy((void*)A9_STUB_ENTRY, firmLaunchStub, 0x200>>2);

	//printf("Starting firm launch...\n");
	void (*stub_entry)(void *entry) = (void*)A9_STUB_ENTRY;
	disableMpu();
	stub_entry(entry);
}
