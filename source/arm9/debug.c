#include <stdio.h>
#include <string.h>
#include "types.h"
#include "mem_map.h"
#include "console.h"

static bool exceptionTriggered = false;

static u32 debugHash = 0;

void NORETURN guruMeditation(u8 type, unsigned int *excStack, unsigned int *stackPointer)
{
	const char *typeStr[3] = {"Undefined instruction", "Prefetch abort", "Data abort"};
	u32 realPc, instSize = 4;
	bool codeChanged = false;

	if(exceptionTriggered)	// inception :(
		while(1);

	exceptionTriggered = true;

	// verify text and rodata
	u32 prevHash = debugHash;
	hashCodeRoData();
	if(prevHash != debugHash)
		codeChanged = true;

	consoleInit(0, NULL);

	if(excStack[1] & 0x20) instSize = 2; // Processor was in Thumb mode?
	if(type == 2) realPc = excStack[15] - (instSize * 2); // Data abort
	else realPc = excStack[15] - instSize; // Other

	printf("\x1b[41m\x1b[0J\x1b[9CGuru Meditation Error!\n\n%s:\nCPSR: 0x%X\n", typeStr[type], excStack[1]);
	printf("r0 = 0x%08X r8  = 0x%08X\n"
			"r1 = 0x%08X r9  = 0x%08X\n"
			"r2 = 0x%08X r10 = 0x%08X\n"
			"r3 = 0x%08X r11 = 0x%08X\n"
			"r4 = 0x%08X r12 = 0x%08X\n"
			"r5 = 0x%08X sp  = 0x%08X\n"
			"r6 = 0x%08X lr  = 0x%08X\n"
			"r7 = 0x%08X pc  = 0x%08X\n\n",
			excStack[2], excStack[10],
			excStack[3], excStack[11],
			excStack[4], excStack[12],
			excStack[5], excStack[13],
			excStack[6], excStack[14],
			excStack[7], (unsigned int)stackPointer,
			excStack[8], excStack[0],
			excStack[9], realPc);

	// make sure we can actually dump the stack without triggering another fault.
	if((stackPointer >= (unsigned int*)A9_RAM_BASE) && (stackPointer < (unsigned int*)(A9_RAM_BASE + A9_RAM_SIZE)))
	{
		puts("Stack dump:");
		for(u32 i = 0; i < 15; i++)
		{
			printf("0x%08X: %08X %08X\n", (unsigned int)stackPointer, stackPointer[0], stackPointer[1]);
			stackPointer += 2;
		}
	}

	if(codeChanged) printf("Attention: RO section data changed!!");

	// avoid fs corruptions
	unmount_fs();
	devs_close();

	while(1);
}

void NORETURN panic()
{
	consoleInit(0, NULL);

	printf("\x1b[41m\x1b[0J\x1b[9CGPANIC!!!\n");
	
	unmount_fs();
	devs_close();
	
	while(1);
}

void hashCodeRoData()
{
	extern u32 *__text_start;
	extern u32 *__exidx_start;
	
	u32 *start = __text_start;
	u32 *end = __exidx_start;
	
	debugHash = 0;
	
	for(u32 *ptr = start; ptr < end; ptr++)
		debugHash += *ptr;
}
