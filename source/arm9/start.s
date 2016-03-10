#include "mem_map.h"

.arm
.arch armv5te
.fpu softvfp

.global _start
.global _init

.type _start STT_FUNC
.type _init STT_FUNC

.section .init



_start:
	mrs r0, cpsr
	orr r0, r0, #0xC0           @ Disable all interrupts
	msr cpsr_c, r0

	ldr sp, =(A9_STUB_ENTRY-8)

	bl flushDCache             @ Flush/invalidate caches to make sure nothing goes wrong
	bl invalidateICache        @ if we are loaded from a bad loader.
	bl invalidateDCache
	bl setupMpu

	ldr r0, =(CORE_SYNC_ID & 0xFFFFFFF0)
	mov r1, #0
	str r1, [r0, #0xC]          @ Clear arm9 communication fields
	str r1, [r0, #0x8]

	bl bss_clear
	blx heap_init
	blx __libc_init_array
	
	blx main

endlessLoop:
	mov r0, #0
	mcr p15, 0, r0, c7, c0, 4  @ Wait for interrupt
	b endlessLoop


bss_clear:
	ldr r1, =__bss_start__
	ldr r2, =__bss_end__
	mov r3, #0

	loop_clear:
	cmp r1, r2
	bxeq lr
	strb r3, [r1]
	add r1, r1, #1
	b loop_clear


@ needed by libc
_init:
	bx lr
