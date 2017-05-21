#include "mem_map.h"

.arm
.cpu mpcore
.fpu vfpv2

.global _start
.global clearMem
.global _init
.global deinitCpu

.type vectors STT_FUNC
.type _start STT_FUNC
.type stubExceptionVectors STT_FUNC
.type clearMem STT_FUNC
.type setupVfp STT_FUNC
.type _init STT_FUNC
.type deinitCpu STT_FUNC

.extern __bss_start__
.extern __bss_end__
.extern setupMmu
.extern __libc_init_array
.extern main
.extern firm_launch

.section ".crt0"



vectors:
	ldr pc, =_start            @ Reset vector
	ldr pc, =(vectors + 0x04)  @ Undefined instruction vector
	ldr pc, =(vectors + 0x08)  @ Software interrupt (SVC) vector
	ldr pc, =(vectors + 0x0C)  @ Prefetch abort vector
	ldr pc, =(vectors + 0x10)  @ Data abort vector
	ldr pc, =(vectors + 0x14)  @ Reserved (unused) vector
	ldr pc, =(vectors + 0x18)  @ Interrupt (IRQ) vector
	ldr pc, =(vectors + 0x1C)  @ Fast interrupt (FIQ) vector
.pool


_start:
	cpsid aif, #0x13           @ Disable all interrupts, SVC mode

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
	mcr p15, 0, r0, c7, c5, 0   @ Invalidate Entire Instruction Cache. Also flushes the branch target cache
	mcr p15, 0, r0, c7, c6, 0   @ Invalidate Entire Data Cache
	mcr p15, 0, r0, c7, c10, 4  @ Data Synchronization Barrier

	bl stubExceptionVectors     @ Stub the vectors in AXIWRAM bootrom vectors jump to

	mov sp, #0                  @ SVC mode sp (Unused, aborts)
	cpsid aif, #0x11            @ FIQ mode
	mov sp, #0                  @ Not yet
	cpsid aif, #0x12            @ IRQ mode
	mov sp, #0                  @ Not yet
	cpsid aif, #0x1F            @ System mode
	ldr sp, =A11_STACK_END

	@ Clear bss section
	ldr r0, =__bss_start__
	ldr r1, =__bss_end__
	sub r1, r1, r0
	bl clearMem

	blx setupMmu
	bl setupVfp
	clrex
	cpsie a

	blx __libc_init_array      @ Initialize ctors and dtors

	mov r0, #0                 @ argc
	mov r1, #0                 @ argv
	blx main
	cmp r0, #0
	bne fail_loop
	bl deinitCpu
	b firm_launch
	fail_loop:
		wfi
		b fail_loop
.pool


#define MAKE_BRANCH(src, dst) (0xEA000000 | (((((dst) - (src)) >> 2) - 2) & 0xFFFFFF))

stubExceptionVectors:
	ldr r0, =A11_VECTORS_START
	mov r1, #6
	ldr r2, =MAKE_BRANCH(0, 0)  @ Endless loop
	stubExceptionVectors_lp:
		str r2, [r0], #8
		subs r1, r1, #1
		bne stubExceptionVectors_lp
	bx lr


@ void clearMem(u32 *adr, u32 size)
clearMem:
	mov r2, #0
	bics r12, r1, #31
	sub r1, r1, r12
	beq clearMem_check_zero
	stmfd sp!, {r4-r9}
	mov r3, #0
	mov r4, #0
	mov r5, #0
	mov r6, #0
	mov r7, #0
	mov r8, #0
	mov r9, #0
	clearMem_block_lp:
		stmia r0!, {r2-r9}
		subs r12, r12, #32
		bne clearMem_block_lp
	ldmfd sp!, {r4-r9}
clearMem_check_zero:
	cmp r1, #0
	bxeq lr
	clearMem_remaining_lp:
		str r2, [r0], #4
		subs r1, r1, #4
		bne clearMem_remaining_lp
	bx lr
.pool


setupVfp:
	mov r0, #0
	mov r1, #0xF00000           @ Give full access to cp10/11 in user and privileged mode
	mcr p15, 0, r1, c1, c0, 2   @ Write Coprocessor Access Control Register
	mcr p15, 0, r0, c7, c5, 4   @ Flush Prefetch Buffer
	mov r1, #0x40000000         @ Clear exception bits and enable VFP11
	mov r2, #0x3C00000          @ Round towards zero (RZ) mode, flush-to-zero mode, default NaN mode
	fmxr fpexc, r1              @ Write Floating-point exception register
	fmxr fpscr, r2              @ Write Floating-Point Status and Control Register
	bx lr
.pool


_init:
	bx lr
.pool


deinitCpu:
	cpsid aif, #0x1F            @ System mode
	mov r3, lr

	bl stubExceptionVectors
	bl flushDCache
	mov r2, #0
	mrc p15, 0, r0, c1, c0, 0   @ Read control register
	ldr r1, =0xC03805           @ Disable MMU, D-Cache, Program flow prediction, I-Cache,
	                            @ high exception vectors, Unaligned data access,
	                            @ subpage AP bits disabled
	bic r0, r0, r1
	mcr p15, 0, r0, c1, c0, 0   @ Write control register
	mrc p15, 0, r0, c1, c0, 1   @ Read Auxiliary Control Register
	bic r0, r0, #0x6F           @ Return stack, Dynamic branch prediction, Static branch prediction,
	                            @ Instruction folding, SMP mode: the CPU is taking part in coherency
	                            @ and L1 parity checking
	mcr p15, 0, r0, c1, c0, 1   @ Write Auxiliary Control Register

	mcr p15, 0, r2, c7, c5, 4   @ Flush Prefetch Buffer
	mcr p15, 0, r2, c7, c5, 0   @ Invalidate Entire Instruction Cache. Also flushes the branch target cache
	mcr p15, 0, r2, c7, c6, 0   @ Invalidate Entire Data Cache
	mcr p15, 0, r2, c7, c10, 4  @ Data Synchronization Barrier

	@ Disable VFP11
	fmxr fpscr, r2              @ Write Floating-Point Status and Control Register
	fmxr fpexc, r2              @ Write Floating-point exception register
	clrex
	bx r3
.pool
