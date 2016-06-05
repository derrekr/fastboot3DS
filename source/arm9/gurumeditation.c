#include <stdio.h>
#include <string.h>
#include "types.h"
#include "mem_map.h"
#include "console.h"

void NORETURN guruMeditation(u8 type, u32 *excStack, u32 *stackPointer)
{
	const char *typeStr[3] = {"Undefined instruction", "Prefetch abort", "Data abort"};
	u32 realPc, instSize = 4;


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
			excStack[7], (u32)stackPointer,
			excStack[8], excStack[0],
			excStack[9], realPc);

	// make sure we can actually dump the stack without triggering another fault.
	if((stackPointer >= A9_RAM_BASE) && (stackPointer < A9_RAM_BASE + A9_RAM_SIZE))
	{
		puts("Stack dump:");
		for(u32 i = 0; i < 15; i++)
		{
			printf("0x%08X: %08X %08X\n", (u32)stackPointer, stackPointer[0], stackPointer[1]);
			stackPointer += 2;
		}
	}

	while(1);
}
