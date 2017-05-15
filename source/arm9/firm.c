#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
// we need the arm11 mem map information
#define ARM11
#include "mem_map.h"
#undef ARM11
#include "arm9/firm.h"
#include "arm9/crypto.h"
#include "arm9/ndma.h"
#include "cache.h"
#include "util.h"
#include "pxi.h"



/* We don't want the FIRM to do hacky stuff with our loader */

typedef struct
{
	u32 addr;
	u32 size;
} firmProtectedArea;

static const firmProtectedArea firmProtectedAreas[] = {
	{	// FIRM buffer
		FIRM_LOAD_ADDR, FIRM_MAX_SIZE
	},
	{	// io regs
		IO_MEM_BASE, VRAM_BASE - IO_MEM_BASE
	},
	{	// arm9 exception vector table
		A9_VECTORS_START, A9_VECTORS_SIZE
	},
	{	// arm9 stack
		A9_STACK_START, A9_STACK_END - A9_STACK_START
	},
	{	// arm9 relocated firm launch stub
		A9_STUB_ENTRY, A9_STUB_SIZE
	},
	{	// arm11 exception vector table
		A11_VECTORS_START, A11_VECTORS_SIZE
	},
	{	// arm11 stack
		A11_STACK_START, A11_STACK_END - A11_STACK_START
	},
	{	// arm11 relocated firm launch stub
		A11_STUB_ENTRY, A11_STUB_SIZE
	}	
};

/* Calculates the actual firm partition size by using its header */
bool firm_size(size_t *size)
{
	firm_header *firm_hdr = (firm_header*)FIRM_LOAD_ADDR;
	u32 curLen = sizeof(firm_header);
	u32 curOffset = 0;
	*size = 0;
	
	/* scan sections in reverse order */
	for(int i=3; i>=0; i--)
	{
		firm_sectionheader *section = &firm_hdr->section[i];

		if(section->size == 0)
			continue;
		
		if(section->offset <= curOffset)
			continue;
		
		curOffset = section->offset;
			
		if(section->size > FIRM_MAX_SIZE || curOffset >= FIRM_MAX_SIZE)
			return false;
		
		if(curLen < curOffset + section->size)
			curLen = curOffset + section->size;
		
		if(curLen > FIRM_MAX_SIZE)
			return false;
	}
	
	*size = curLen;
	
	return true;
}

// NOTE: Do not call any functions here!
void NAKED firmLaunchStub(void)
{	
	firm_header *firm_hdr = (firm_header*)FIRM_LOAD_ADDR;
	register u32 entry9 = firm_hdr->entrypointarm9;
	register u32 ret = firm_hdr->entrypointarm9;
	register u32 entry11 = firm_hdr->entrypointarm11;


	for(int i = 0; i < 4; i++)
	{
		firm_sectionheader *section = &firm_hdr->section[i];
		if(section->size == 0)
			continue;

		// Use NDMA for everything but copy method 2
		if(section->copyMethod < 2)
		{
			REG_NDMA_SRC_ADDR(i) = FIRM_LOAD_ADDR + section->offset;
			REG_NDMA_DST_ADDR(i) = section->address;
			REG_NDMA_WRITE_CNT(i) = section->size / 4;
			REG_NDMA_BLOCK_CNT(i) = NDMA_BLOCK_SYS_FREQ;
			REG_NDMA_CNT(i) = NDMA_DST_UPDATE_INC | NDMA_SRC_UPDATE_INC |
			                  NDMA_STARTUP_IMMEDIATE | NDMA_ENABLE;
		}
		else
		{
			u32 *dst = (u32*)section->address;
			u32 *src = (u32*)(FIRM_LOAD_ADDR + section->offset);

			for(u32 n = 0; n < section->size / 4; n += 4)
			{
				dst[n + 0] = src[n + 0];
				dst[n + 1] = src[n + 1];
				dst[n + 2] = src[n + 2];
				dst[n + 3] = src[n + 3];
			}
		}
	}

	while(REG_NDMA0_CNT & NDMA_ENABLE || REG_NDMA1_CNT & NDMA_ENABLE ||
	      REG_NDMA2_CNT & NDMA_ENABLE || REG_NDMA3_CNT & NDMA_ENABLE);

	// Tell ARM11 its entrypoint
	REG_PXI_SYNC9 = 0; // Disable all IRQs
	while(REG_PXI_CNT9 & PXI_SEND_FIFO_FULL);
	REG_PXI_SEND9 = (u32)entry11;

	// Wait for ARM11...
	for(;;)
	{
		while(REG_PXI_CNT9 & PXI_RECV_FIFO_EMPTY);
		if(REG_PXI_RECV9 == PXI_RPL_FIRM_LAUNCH_READY)
			break;
	}

	REG_PXI_CNT9 = 0; // Disable PXI

	// go for it!
	__asm__ __volatile__("mov lr, %0\n\tbx %1" : : "r" (ret), "r" (entry9) : "lr", "pc");
}

bool firm_verify(u32 fwSize, bool skipHashCheck, bool printInfo)
{
	firm_header *firm_hdr = (firm_header*)FIRM_LOAD_ADDR;
	const char *res[2] = {"\x1B[31mBAD", "\x1B[32mGOOD"};
	bool isValid;
	bool retval = true;
	u32 hash[8];
	
	if(fwSize > FIRM_MAX_SIZE)
		return false;
		
	if(fwSize <= sizeof(firm_header))
		return false;
	
	if(memcmp(&firm_hdr->magic, "FIRM", 4) != 0)
		return false;
	
	if(printInfo)
	{
		printf("ARM9  entry: 0x%"PRIX32"\n", firm_hdr->entrypointarm9);
		printf("ARM11 entry: 0x%"PRIX32"\n", firm_hdr->entrypointarm11);
	}
	
	for(u32 i=0; i<4; i++)
	{
		firm_sectionheader *section = &firm_hdr->section[i];

		if(section->size == 0)
			continue;

		if(printInfo)
			printf("Section %i:\noffset: 0x%"PRIX32", addr: 0x%"PRIX32", size: 0x%"PRIX32"\n", (int)i,
				   section->offset, section->address, section->size);
				
		if(section->offset >= fwSize || section->offset < sizeof(firm_header))
		{
			if(printInfo)
				printf("\x1B[31mBad section offset!\e[0m\n");
			return false;
		}
		
		if((section->size >= fwSize) || (section->size + section->offset > fwSize))
		{
			if(printInfo)
				printf("\x1B[31mBad section size!\e[0m\n");
			return false;
		}
		
		// check for bad sections
		const u32 numEntries = arrayEntries(firmProtectedAreas);
		
		for(u32 j=0; j<numEntries; j++)
		{ 
			// protected region dimensions
			u32 addr = firmProtectedAreas[j].addr;
			u32 size = firmProtectedAreas[j].size;
			
			// firmware section dimensions
			u32 start = section->address;
			u32 end = start + section->size;
			
			isValid = true;
			
			if(start >= addr && start < addr + size) isValid = false;
			
			else if(end > addr && end <= addr + size) isValid = false;
			
			else if(start < addr && end > addr + size) isValid = false;
			
			if(!isValid)
			{
				if(printInfo)
					printf("\x1B[31mUnallowed section:\n0x%"PRIX32" - 0x%"PRIX32"\e[0m\n", start, end);
				retval = false;
				break;
			}
		}
		
		if(!skipHashCheck)
		{
			sha((u32*)(FIRM_LOAD_ADDR + section->offset), section->size, hash,
							SHA_INPUT_BIG | SHA_MODE_256, SHA_OUTPUT_BIG);
			isValid = memcmp(hash, section->hash, 32) == 0;
			
			if(printInfo)
				printf("Hash: %s\e[0m\n", res[isValid]);
			
			retval &= isValid;
		}
	}
	
	return retval;
}

noreturn void firm_launch(void)
{
	//printf("Sending PXI_CMD_FIRM_LAUNCH\n");
	PXI_sendWord(PXI_CMD_FIRM_LAUNCH);

	//printf("Waiting for ARM11...\n");
	while(PXI_recvWord() != PXI_RPL_OK);
	
	//printf("Relocating FIRM launch stub...\n");
	NDMA_copy((u32*)A9_STUB_ENTRY, (u32*)firmLaunchStub, A9_STUB_SIZE>>2);

	printf("Starting firm launch...\n");
	void (*stub)(void) = (void (*)(void))A9_STUB_ENTRY;
	stub();
	while(1);
}
