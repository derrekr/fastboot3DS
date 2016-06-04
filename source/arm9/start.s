#include "mem_map.h"

.arm
.arch armv5te
.fpu softvfp

.global _start
.global _init

.type _start STT_FUNC
.type _init STT_FUNC

.extern setupSystem
.extern __libc_init_array
.extern heap_init
.extern main
.extern __bss_start__
.extern __bss_end__

.section .init



_start:
	msr cpsr_c, #0xDF           @ Disable all interrupts, system mode
	ldr sp, =(A9_STUB_ENTRY-8)

	bl setupSystem

	ldr r0, =(CORE_SYNC_ID & 0xFFFFFFF0)
	mov r1, #0
	str r1, [r0, #0x8]          @ Clear arm9 communication fields
	str r1, [r0, #0xC]

	ldr r0, =__bss_start__
	ldr r1, =__bss_end__
	sub r1, r1, r0
	mov r2, #0
	loop_bss_clear:
		str r2, [r0], #4
		subs r1, r1, #4
		bne loop_bss_clear

	blx __libc_init_array
	blx heap_init
	blx main
	b .                         @ If main ever returns loop forever


@ needed by libc
_init:
	bx lr
