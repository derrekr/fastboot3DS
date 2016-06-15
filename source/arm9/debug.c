#include <stdio.h>
#include <string.h>
#include "types.h"
#include "mem_map.h"
#include "console.h"



static u32 debugHash = 0;


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

void NORETURN panic()
{
	consoleInit(0, NULL);

	printf("\x1b[41m\x1b[0J\x1b[9CGPANIC!!!\n");
	
	unmount_fs();
	devs_close();
	
	while(1);
}

void NORETURN guruMeditation(u8 type, u32 *excStack, u32 *stackPointer)
{
	const char *typeStr[3] = {"Undefined instruction", "Prefetch abort", "Data abort"};
	static bool exceptionTriggered = false;
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

	printf("\x1b[41m\x1b[0J\x1b[9CGuru Meditation Error!\n\n%s:\n", typeStr[type]);
	printf("CPSR: 0x%08"PRIX32"\n"
			"r0 = 0x%08"PRIX32" r8  = 0x%08"PRIX32"\n"
			"r1 = 0x%08"PRIX32" r9  = 0x%08"PRIX32"\n"
			"r2 = 0x%08"PRIX32" r10 = 0x%08"PRIX32"\n"
			"r3 = 0x%08"PRIX32" r11 = 0x%08"PRIX32"\n"
			"r4 = 0x%08"PRIX32" r12 = 0x%08"PRIX32"\n"
			"r5 = 0x%08"PRIX32" sp  = 0x%08"PRIX32"\n"
			"r6 = 0x%08"PRIX32" lr  = 0x%08"PRIX32"\n"
			"r7 = 0x%08"PRIX32" pc  = 0x%08"PRIX32"\n\n",
			excStack[1],
			excStack[2], excStack[10],
			excStack[3], excStack[11],
			excStack[4], excStack[12],
			excStack[5], excStack[13],
			excStack[6], excStack[14],
			excStack[7], (u32)stackPointer,
			excStack[8], excStack[0],
			excStack[9], realPc);

	// make sure we can actually dump the stack without triggering another fault.
	if((stackPointer >= (u32*)A9_RAM_BASE) && (stackPointer < (u32*)(A9_RAM_BASE + A9_RAM_SIZE)))
	{
		puts("Stack dump:");
		for(u32 i = 0; i < 15; i++)
		{
			printf("0x%08"PRIX32": %08"PRIX32" %08"PRIX32"\n", (u32)stackPointer, stackPointer[0], stackPointer[1]);
			stackPointer += 2;
		}
	}

	if(codeChanged) printf("Attention: RO section data changed!!");

	// avoid fs corruptions
	unmount_fs();
	devs_close();

	while(1);
}
