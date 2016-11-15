#include "mem_map.h"
#include "version.h"

.arm
.arch armv5te
.fpu softvfp

.global _start
.global clearMem
.global _init

.type _start STT_FUNC
.type clearMem STT_FUNC
.type _init STT_FUNC

.extern initCpu
.extern deinitCpu
.extern __libc_init_array
.extern main
.extern firm_launch
.extern __bss_start__
.extern __bss_end__
.extern __end__
.extern fake_heap_start
.extern fake_heap_end

.section ".crt0"



_start:
	msr cpsr_cxsf, #0xDF           @ Disable all interrupts, system mode
	b skip_pool

	.string "3DS BOOTLOADER "
	.word   BOOTLOADER_VERSION

skip_pool:
	bl initCpu

	ldr sp, =A9_STACK_END

	@ Clear bss section
	ldr r0, =__bss_start__
	ldr r1, =__bss_end__
	bl clearMem

	@ Setup newlib heap
	ldr r0, =__end__
	add r0, r0, #4              @ Do not overwrite the value at symbol __end__
	ldr r1, =fake_heap_start
	str r0, [r1]
	add r0, r0, #A9_HEAP_SIZE
	ldr r1, =fake_heap_end
	str r0, [r1]

	blx __libc_init_array       @ Initialize ctors and dtors

	mov r0, #0                  @ argc
	mov r1, #0                  @ argv
	blx main
	cmp r0, #0
	bne .
	bl deinitCpu
	b firm_launch
.pool


@ void clearMem(u32 *start, u32 *end)
clearMem:
	sub r1, r1, r0
	mov r2, #0
	clearMem_lp:
		str r2, [r0], #4
		subs r1, r1, #4
		bne clearMem_lp
	bx lr


@ Needed by libc
_init:
	bx lr
