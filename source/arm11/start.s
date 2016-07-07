#include "mem_map.h"

.arm
.cpu mpcore
.fpu softvfp

.global _start
.global _init
.global disableCaches

.type _start STT_FUNC
.type _init STT_FUNC
.type disableCaches STT_FUNC

.extern main
.extern __bss_start__
.extern __bss_end__

.section ".crt0"



_start:
	cpsid aif, #0x1F           @ Disable all interrupts, system mode

	ldr r0, =0x54078           @ Everything disabled
	mcr p15, 0, r0, c1, c0, 0  @ Write control register
	mov r1, #0                 @ Everything disabled
	mcr p15, 0, r1, c1, c0, 1  @ Write Auxiliary Control Register
	mcr p15, 0, r1, c7, c5, 4  @ Flush Prefetch Buffer
	mcr p15, 0, r1, c7, c5, 0  @ Invalidate Entire Instruction Cache. Also flushes the branch target cache
	mcr p15, 0, r1, c7, c6, 0  @ Invalidate Entire Data Cache
	mcr p15, 0, r1, c7, c10, 4 @ Data Synchronization Barrier

	mov r0, #0x2F              @ Enable Return stack, Dynamic branch prediction, Static branch prediction,
                               @ Instruction folding, SMP mode, the CPU is taking part in coherency
	mcr p15, 0, r0, c1, c0, 1  @ Write Auxiliary Control Register
	ldr r0, =0x5587C           @ Enable D-Cache, program flow prediction and I-Cache
	mcr p15, 0, r0, c1, c0, 0  @ Write control register
	mcr p15, 0, r1, c7, c5, 4  @ Flush Prefetch Buffer
	mcr p15, 0, r1, c7, c5, 0  @ Invalidate Entire Instruction Cache. Also flushes the branch target cache
	mcr p15, 0, r1, c7, c6, 0  @ Invalidate Entire Data Cache
	mcr p15, 0, r1, c7, c10, 4 @ Data Synchronization Barrier

	// Set sp
	ldr sp, =A11_STACK_END

	ldr r0, =__bss_start__
	ldr r1, =__bss_end__
	sub r1, r1, r0
	mov r2, #0
	loop_bss_clear:
		str r2, [r0], #4
		subs r1, r1, #4
		bne loop_bss_clear

	blx main
	b .                        @ If main ever returns loop forever


disableCaches:
	mov r2, lr
	bl flushDCache

	ldr r0, =0x54078           @ Everything disabled
	mcr p15, 0, r0, c1, c0, 0  @ Write control register
	mov r0, #0                 @ Everything disabled
	mcr p15, 0, r0, c1, c0, 1  @ Write Auxiliary Control Register

	mcr p15, 0, r0, c7, c5, 4  @ Flush Prefetch Buffer
	mcr p15, 0, r0, c7, c5, 0  @ Invalidate Entire Instruction Cache. Also flushes the branch target cache
	mcr p15, 0, r0, c7, c6, 0  @ Invalidate Entire Data Cache
	mcr p15, 0, r0, c7, c10, 4 @ Data Synchronization Barrier
	bx r2


_init:
	bx lr
