/*
 *   This file is part of fastboot 3DS
 *   Copyright (C) 2017 derrek, profi200
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "arm.h"
#include "mem_map.h"

.arm
.cpu mpcore
.fpu vfpv2

.global _start
.global clearMem
.global _init
.global deinitCpu

.type vectors %function
.type _start %function
.type stubExceptionVectors %function
.type clearMem %function
.type setupVfp %function
.type _init %function
.type deinitCpu %function

.extern irqHandler
.extern undefInstrHandler
.extern prefetchAbortHandler
.extern dataAbortHandler
.extern __bss_start__
.extern __bss_end__
.extern __end__
.extern fake_heap_start
.extern fake_heap_end
.extern setupMmu
.extern __libc_init_array
.extern core123Init
.extern systemInit
.extern main

.section ".crt0", "ax"



__start__:
vectors:
	ldr pc, resetHandlerPtr         @ Reset vector
	ldr pc, undefInstrHandlerPtr    @ Undefined instruction vector
	ldr pc, svcHandlerPtr           @ Software interrupt (SVC) vector
	ldr pc, prefetchAbortHandlerPtr @ Prefetch abort vector
	ldr pc, dataAbortHandlerPtr     @ Data abort vector
	ldr pc, reservedHandlerPtr      @ Reserved (unused) vector
	ldr pc, irqHandlerPtr           @ Interrupt (IRQ) vector
	ldr pc, fiqHandlerPtr           @ Fast interrupt (FIQ) vector
	resetHandlerPtr:         .word _start
	undefInstrHandlerPtr:    .word undefInstrHandler
	svcHandlerPtr:           .word (vectors + 0x08)
	prefetchAbortHandlerPtr: .word prefetchAbortHandler
	dataAbortHandlerPtr:     .word dataAbortHandler
	reservedHandlerPtr:      .word (vectors + 0x14)
	irqHandlerPtr:           .word irqHandler
	fiqHandlerPtr:           .word (vectors + 0x1C)


_start:
	cpsid aif, #PSR_SVC_MODE

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
	mcr p15, 0, r0, c7, c7, 0   @ Invalidate Both Caches. Also flushes the branch target cache
	mcr p15, 0, r0, c7, c10, 4  @ Data Synchronization Barrier
	mcr p15, 0, r0, c7, c5, 4   @ Flush Prefetch Buffer
	clrex

	mrc p15, 0, r4, c0, c0, 5   @ Get CPU ID
	ands r4, r4, #3
	bleq stubExceptionVectors   @ Stub the vectors in AXIWRAM bootrom vectors jump to

	mov sp, #0                  @ unused SVC mode sp
	cps #PSR_FIQ_MODE
	mov sp, #0                  @ Unused
	cps #PSR_IRQ_MODE
	mov sp, #0                  @ not needed
	cps #PSR_ABORT_MODE
	ldr r0, =A11_EXC_STACK_END
	mov sp, r0
	cps #PSR_UNDEF_MODE
	mov sp, r0
	cps #PSR_SYS_MODE
	adr r2, _sysmode_stacks
	ldr sp, [r2, r4, lsl #2]

	cmp r4, #0
	bne _start_skip_bss_init_array

	@ Clear bss section
	ldr r0, =__bss_start__
	ldr r1, =__bss_end__
	sub r1, r1, r0
	bl clearMem
	@ Setup newlib heap
	ldr r0, =A11_HEAP_END
	ldr r1, =fake_heap_end
	str r0, [r1]
	blx __libc_init_array       @ Initialize ctors and dtors
#ifdef CORE123_INIT
	blx core123Init
#endif
_start_skip_bss_init_array:
	ldr r2, =0x706              @ Disable + reset all counters. Cycle counter divider 1. IRQs disabled.
	mcr p15, 0, r2, c15, c12, 0 @ Write Performance Monitor Control Register
	blx setupMmu
	bl setupVfp
	cpsie a
	blx systemInit

	mov r0, #0                  @ argc
	mov r1, #0                  @ argv
	blx main
	_start_lp:
		wfi
		b _start_lp


#define MAKE_BRANCH(src, dst) (0xEA000000 | (((((dst) - (src)) >> 2) - 2) & 0xFFFFFF))

stubExceptionVectors:
	ldr r0, =A11_VECTORS_START
	ldr r2, =MAKE_BRANCH(0, 0)  @ Endless loop
	mov r1, #6
	stubExceptionVectors_lp:
		str r2, [r0], #8
		subs r1, r1, #1
		bne stubExceptionVectors_lp
	bx lr


@ void clearMem(u32 *adr, u32 size)
clearMem:
	bics r12, r1, #31
	mov r2, #0
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


setupVfp:
	mov r0, #0
	mov r1, #0xF00000           @ Give full access to cp10/11 in user and privileged mode
	mov r2, #0x40000000         @ Clear exception bits and enable VFP11
	mov r3, #0x3C00000          @ Round towards zero (RZ) mode, flush-to-zero mode, default NaN mode
	mcr p15, 0, r1, c1, c0, 2   @ Write Coprocessor Access Control Register
	mcr p15, 0, r0, c7, c5, 4   @ Flush Prefetch Buffer
	fmxr fpexc, r2              @ Write Floating-point exception register
	fmxr fpscr, r3              @ Write Floating-Point Status and Control Register
	bx lr


_init:
	bx lr


deinitCpu:
	mov r3, lr

	cpsid aif, #PSR_SYS_MODE
	bl stubExceptionVectors
	bl flushDCache
	mov r2, #0
	@ Disable VFP11
	fmxr fpscr, r2              @ Write Floating-Point Status and Control Register
	fmxr fpexc, r2              @ Write Floating-point exception register

	ldr r1, =0xC03805           @ Disable MMU, D-Cache, Program flow prediction, I-Cache,
	                            @ high exception vectors, Unaligned data access,
	                            @ subpage AP bits disabled
	mrc p15, 0, r0, c1, c0, 0   @ Read control register
	bic r0, r0, r1
	mcr p15, 0, r0, c1, c0, 0   @ Write control register
	mrc p15, 0, r0, c1, c0, 1   @ Read Auxiliary Control Register
	bic r0, r0, #0x6F           @ Return stack, Dynamic branch prediction, Static branch prediction,
	                            @ Instruction folding, SMP mode: the CPU is taking part in coherency
	                            @ and L1 parity checking
	mcr p15, 0, r0, c1, c0, 1   @ Write Auxiliary Control Register
	mcr p15, 0, r2, c7, c7, 0   @ Invalidate Both Caches. Also flushes the branch target cache
	mcr p15, 0, r2, c7, c10, 4  @ Data Synchronization Barrier
	mcr p15, 0, r2, c7, c5, 4   @ Flush Prefetch Buffer
	bx r3


_sysmode_stacks:
	.word A11_C0_STACK_END      @ Stack for core 0
	.word A11_C1_STACK_END      @ Stack for core 1
	.word A11_C2_STACK_END      @ Stack for core 2
	.word A11_C3_STACK_END      @ Stack for core 3
.pool
