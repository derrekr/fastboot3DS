#include "mem_map.h"

.arm
.cpu mpcore
.fpu vfpv2

.global initCpu
.global deinitCpu

.type initCpu STT_FUNC
.type setupExceptionVectors STT_FUNC
.type setupVfp STT_FUNC
.type deinitCpu STT_FUNC

.extern setupMmu
.extern flushDCache

.section ".init"



initCpu:
	mov r10, lr

	@ Control register:
	@ [29] Force AP functionality             : disabled
	@ [28] TEX remap                          : disabled
	@ [27] NMFI bit                           : normal FIQ behavior
	@ [25] CPSR E bit on taking an exception  : 0
	@ [23] Extended page table configuration  : subpage AP bits enabled
	@ [22] Unaligned data access              : disabled
	@ [15] Disable loading TBIT               : disabled
	@ [13] Vector select                      : 0x00000000
	@ [12] Level one instruction cache        : disabled
	@ [11] Program flow prediction            : disabled
	@ [7]  Endianess                          : little
	@ [2]  Level one data cache               : disabled
	@ [1]  Strict data address alignment fault: disabled
	@ [0]  MMU                                : disabled
	ldr r0, =0x54078
	mcr p15, 0, r0, c1, c0, 0   @ Write control register
	mov r0, #0
	mcr p15, 0, r0, c1, c0, 1   @ Write Auxiliary Control Register

	mcr p15, 0, r0, c7, c5, 4   @ Flush Prefetch Buffer
	mcr p15, 0, r0, c7, c7, 0   @ Invalidate Both Caches. Also flushes the branch target cache
	mcr p15, 0, r0, c7, c10, 4  @ Data Synchronization Barrier
	clrex

	ldr sp, =A11_STACK_END

	bl setupExceptionVectors    @ Setup the vectors in AXIWRAM bootrom vectors jump to
	bl setupVfp
	blx setupMmu
	bx r10
.pool


#define MAKE_BRANCH(src, dst) (0xEA000000 | (((((dst) - (src)) >> 2) - 2) & 0xFFFFFF))

setupExceptionVectors:
	adr r0, __vectorStubs
	ldr r1, =A11_VECTORS_START
	ldmia r0!, {r2-r9}
	stmia r1!, {r2-r9}
	ldmia r0, {r2-r5}
	stmia r1, {r2-r5}
	bx lr
.pool
__vectorStubs:
	.word MAKE_BRANCH(A11_VECTORS_START + 0x00, A11_VECTORS_START + 0x00) // IRQ
	.word 0
	.word MAKE_BRANCH(A11_VECTORS_START + 0x08, A11_VECTORS_START + 0x08) // FIQ
	.word 0
	.word MAKE_BRANCH(A11_VECTORS_START + 0x10, A11_VECTORS_START + 0x10) // SVC
	.word 0
	.word MAKE_BRANCH(A11_VECTORS_START + 0x18, A11_VECTORS_START + 0x18) // Undefined instruction
	.word 0
	.word MAKE_BRANCH(A11_VECTORS_START + 0x20, A11_VECTORS_START + 0x20) // Prefetch abort
	.word 0
	.word MAKE_BRANCH(A11_VECTORS_START + 0x28, A11_VECTORS_START + 0x28) // Data abort
	.word 0


setupVfp:
	mov r0, #0
	mov r1, #0x00F00000         @ Give full access to cp10/11 in user and privileged mode
	mcr p15, 0, r1, c1, c0, 2   @ Write Coprocessor Access Control Register
	mcr p15, 0, r0, c7, c5, 4   @ Flush Prefetch Buffer
	mov r1, #0x40000000         @ Clear exception bits and enable VFP11
	mov r2, #0x03C00000         @ Round towards zero (RZ) mode, flush-to-zero mode, default NaN mode
	fmxr fpexc, r1              @ Write Floating-point exception register
	fmxr fpscr, r2              @ Write Floating-Point Status and Control Register
	bx lr
.pool


deinitCpu:
	mov r4, lr

	@ Stub vectors to endless loops
	ldr r0, =A11_VECTORS_START
	mov r1, #6
	ldr r2, =MAKE_BRANCH(0, 0)  @ Endless loop
	deinitCpu_vector_lp:
		str r2, [r0], #8
		subs r1, r1, #1
		bne deinitCpu_vector_lp

	bl flushDCache
	mov r2, #0
	mrc p15, 0, r0, c1, c0, 0   @ Read control register
	ldr r1, =0x803805           @ MMU, D-Cache, Program flow prediction, I-Cache,
                                @ high exception vectors, subpage AP bits disabled
	bic r0, r0, r1
	mcr p15, 0, r0, c1, c0, 0   @ Write control register
	mrc p15, 0, r0, c1, c0, 1   @ Read Auxiliary Control Register
	bic r0, r0, #0x7F           @ Return stack, Dynamic branch prediction, Static branch prediction,
                                @ Instruction folding, L1 and L2 caches are exclusive,
                                @ SMP mode: the CPU is taking part in coherency and L1 parity checking
	mcr p15, 0, r0, c1, c0, 1   @ Write Auxiliary Control Register

	mcr p15, 0, r2, c7, c5, 4   @ Flush Prefetch Buffer
	mcr p15, 0, r2, c7, c7, 0   @ Invalidate Both Caches. Also flushes the branch target cache
	mcr p15, 0, r2, c7, c10, 4  @ Data Synchronization Barrier
	clrex
	@ Disable VFP11
	fmxr fpscr, r2              @ Write Floating-Point Status and Control Register
	fmxr fpexc, r2              @ Write Floating-point exception register
	bx r4
.pool
