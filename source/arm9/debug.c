#include <stdio.h>
#include <string.h>
#include "types.h"
#include "mem_map.h"
#include "pxi.h"
#include "arm9/console.h"
#include "fatfs/ff.h"
#include "arm9/main.h"
#include "arm9/interrupt.h"

static u32 debugHash = 0;


void debugHashCodeRoData()
{
#ifndef NDEBUG
	extern u32 *__start__;
	extern u32 *__exidx_start;
	
	u32 *start = __start__;
	u32 *end = __exidx_start;
	
	debugHash = 0;
	
	for(u32 *ptr = start; ptr < end; ptr++)
		debugHash += *ptr;
#endif
}

noreturn void panic()
{
	register u32 lr __asm__("lr");

	consoleInit(0, NULL);

	printf("\x1b[41m\x1b[0J\x1b[9C****PANIC!!!****\n\nlr = 0x%08" PRIX32 "\n", lr);
	
	unmount_fs();
	devs_close();
	
	PXI_sendWord(PXI_CMD_ALLOW_POWER_OFF);

	for(;;) waitForIrq();
}

noreturn void panicMsg(const char *msg)
{
	register u32 lr __asm__("lr");

	consoleInit(0, NULL);

	printf("\x1b[41m\x1b[0J\x1b[9C****PANIC!!!****\n\nlr = 0x%08" PRIX32 "\n", lr);
	printf("\nERROR MESSAGE:\n%s\n", msg);
	
	unmount_fs();
	devs_close();
	
	PXI_sendWord(PXI_CMD_ALLOW_POWER_OFF);

	for(;;) waitForIrq();
}

// Expects the registers in the exception stack to be in the following order:
// cpsr, pc (unmodified), r0-r14
noreturn void guruMeditation(u8 type, u32 *excStack)
{
	const char *typeStr[3] = {"Undefined instruction", "Prefetch abort", "Data abort"};
	const size_t maxStackWords = 15 * 2;
	u32 realPc, instSize = 4;
	bool codeChanged = false;


	// verify text and rodata
	u32 prevHash = debugHash;
	debugHashCodeRoData();
	if(prevHash != debugHash)
		codeChanged = true;

	consoleInit(0, NULL);

	if(excStack[0] & 0x20) instSize = 2; // Processor was in Thumb mode?
	if(type == 2) realPc = excStack[1] - (instSize * 2); // Data abort
	else realPc = excStack[1] - instSize; // Other

	printf("\x1b[41m\x1b[0J\x1b[9CGuru Meditation Error!\n\n%s:\n", typeStr[type]);
	printf("CPSR: 0x%08" PRIX32 "\n"
	       "r0 = 0x%08" PRIX32 " r8  = 0x%08" PRIX32 "\n"
	       "r1 = 0x%08" PRIX32 " r9  = 0x%08" PRIX32 "\n"
	       "r2 = 0x%08" PRIX32 " r10 = 0x%08" PRIX32 "\n"
	       "r3 = 0x%08" PRIX32 " r11 = 0x%08" PRIX32 "\n"
	       "r4 = 0x%08" PRIX32 " r12 = 0x%08" PRIX32 "\n"
	       "r5 = 0x%08" PRIX32 " sp  = 0x%08" PRIX32 "\n"
	       "r6 = 0x%08" PRIX32 " lr  = 0x%08" PRIX32 "\n"
	       "r7 = 0x%08" PRIX32 " pc  = 0x%08" PRIX32 "\n\n",
	       excStack[0],
	       excStack[2], excStack[10],
	       excStack[3], excStack[11],
	       excStack[4], excStack[12],
	       excStack[5], excStack[13],
	       excStack[6], excStack[14],
	       excStack[7], excStack[15],
	       excStack[8], excStack[16],
	       excStack[9], realPc);

	// make sure we can actually dump the stack without triggering another fault.
	if((excStack[15] >= (u32)A9_STACK_START) && (excStack[15] < (u32)A9_STACK_END) &&
	   (excStack[15] + maxStackWords * 4 < (u32)A9_STACK_END))
	{
		puts("Stack dump:");
		for(u32 i = 0; i < maxStackWords / 2; i++)
		{
			printf("0x%08" PRIX32 ": %08" PRIX32 " %08" PRIX32 "\n", excStack[15],
			       ((u32*)excStack[15])[0], ((u32*)excStack[15])[1]);
			excStack[15] += 8;
		}
	}

	if(codeChanged) printf("Attention: RO section data changed!!");

	// avoid fs corruptions
	unmount_fs();
	devs_close();

	PXI_sendWord(PXI_CMD_ALLOW_POWER_OFF);

	for(;;) waitForIrq();
}

void dumpMem(u8 *mem, u32 size, char *filepath)
{
	FIL file;
	UINT bytesWritten;

	if(f_open(&file, filepath, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
		return;
		
	f_write(&file, mem, size, &bytesWritten);

	f_close(&file);
}
