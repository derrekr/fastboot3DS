#include "mem_map.h"

.arm
.cpu mpcore

.global _start
.global _init
.global flushDCache
.global invalidateICache

.type _start STT_FUNC
.type _init STT_FUNC
.type flushDCache STT_FUNC
.type invalidateICache STT_FUNC

.section .init

_start:
	@ Disable all interrupts and enter svc mode
	cpsid aif, #0x13

	ldr sp, =(0x1FFFFE00-8)    @ Stack starts at the end of AXIWRAM
                               @ before the FIRM launch stub

	ldr r0, =0x1FFFFFF0
	mov r1, #0
	str r1, [r0, #0xC]         @ Clear arm9 communication fields
	str r1, [r0, #0x8]

	bl bss_clear

	mrc p15, 0, r0, c1, c0, 0  @ Read control register
	ldr r1, =0x1004            @ D-Cache and I-Cache bitmask
	orr r0, r0, r1             @ Enable D-Cache and I-Cache
	mcr p15, 0, r0, c1, c0, 0  @ Write control register
	mov r0, #0
	mcr p15, 0, r0, c7, c14, 0 @ Clear and invalidate entire data cache
	mcr p15, 0, r0, c7, c10, 5 @ Data memory barrier
	mcr p15, 0, r0, c7, c5, 0  @ Invalidate entire instruction cache,
                               @ also flushes the branch target cache
	mcr p15, 0, r0, c7, c5, 4  @ Flush prefetch buffer
	mcr p15, 0, r0, c7, c5, 6  @ Flush entire branch target cache
	mcr p15, 0, r0, c7, c10, 4 @ Data synchronization barrier

	blx main
	cmp r0, #0
	bne endlessLoop

	bl flushDCache
	bl invalidateICache
	mrc p15, 0, r0, c1, c0, 0  @ Read control register
	ldr r1, =0x1004            @ D-Cache and I-Cache bitmask
	bic r0, r0, r1             @ Disable D-Cache and I-Cache
	mcr p15, 0, r0, c1, c0, 0  @ Write control register
	ldr pc, =A11_STUB_ENTRY

endlessLoop:
	wfi                        @ Wait for interrupt
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

flushDCache:
	@ Clear and Invalidate Entire Data Cache
	mov r0, #0
	mcr p15, 0, r0, c7, c14, 0
	bx lr

invalidateICache:
	@ Invalidate Entire Instruction Cache,
	@ also flushes the branch target cache
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0
	bx lr

.pool


_init:
	bx lr
