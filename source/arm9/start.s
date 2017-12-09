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

#include "mem_map.h"

.arm
.cpu arm946e-s
.fpu softvfp

.global _start
.global clearMem
.global _init
.global deinitCpu

.type _start %function
.type setupExceptionVectors %function
.type setupTcms %function
.type clearMem %function
.type setupMpu %function
.type _init %function
.type deinitCpu %function

.extern __bss_start__
.extern __bss_end__
.extern __end__
.extern fake_heap_start
.extern fake_heap_end
.extern __libc_init_array
.extern main
.extern irqHandler
.extern undefInstrHandler
.extern prefetchAbortHandler
.extern dataAbortHandler

.section ".crt0", "ax"



__start__:
	.string "fastboot3DS    "
	.word   (VERS_MAJOR<<16 | VERS_MINOR)

_start:
	msr cpsr_cxsf, #0xD3         @ Disable all interrupts, SVC mode

	@ Control register:
	@ [19] ITCM load mode         : disabled
	@ [18] ITCM                   : disabled
	@ [17] DTCM load mode         : disabled
	@ [16] DTCM                   : disabled
	@ [15] Disable loading TBIT   : disabled
	@ [14] Round-robin replacement: disabled
	@ [13] Vector select          : 0xFFFF0000
	@ [12] I-Cache                : disabled
	@ [7]  Endianess              : little
	@ [2]  D-Cache                : disabled
	@ [0]  MPU                    : disabled
	ldr r0, =0x2078
	mcr p15, 0, r0, c1, c0, 0   @ Write control register
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0   @ Invalidate I-Cache
	mcr p15, 0, r0, c7, c6, 0   @ Invalidate D-Cache
	mcr p15, 0, r0, c7, c10, 4  @ Drain write buffer

	bl setupExceptionVectors    @ Setup the vectors in ARM9 mem bootrom vectors jump to
	bl setupTcms                @ Setup and enable DTCM and ITCM

	mov sp, #0                  @ SVC mode sp (Unused, aborts)
	msr cpsr_cxsf, #0xD7        @ Abort mode
	mov sp, #A9_EXC_STACK_END
	msr cpsr_cxsf, #0xDB        @ Undefined mode
	mov sp, #A9_EXC_STACK_END
	msr cpsr_cxsf, #0xD2        @ IRQ mode
	ldr sp, =A9_IRQ_STACK_END
	msr cpsr_cxsf, #0xDF        @ System mode
	ldr sp, =A9_STACK_END

	bl setupMpu

	@ Clear bss section
	ldr r0, =__bss_start__
	ldr r1, =__bss_end__
	sub r1, r1, r0
	bl clearMem
	@ Setup newlib heap
	mov r0, #IO_MEM_BASE        @ CFG9 regs
	ldr r1, [r0, #0xFFC]        @ REG_CFG9_MPCORECFG
	tst r1, #2                  @ Test for New 3DS bit
	movne r1, #1
	strne r1, [r0, #0x200]      @ REG_CFG9_EXTMENTCNT9
	mov r0, #A9_HEAP_END
	addne r0, #A9_RAM_N3DS_EXT_SIZE
	ldr r1, =fake_heap_end
	str r0, [r1]
	blx __libc_init_array       @ Initialize ctors and dtors

	mov r0, #0                  @ argc
	mov r1, #0                  @ argv
	blx main
	_start_lp:
		mov r0, #0
		mcr p15, 0, r0, c7, c0, 4 @ Wait for interrupt
		b _start_lp


#define MAKE_BRANCH(src, dst) (0xEA000000 | (((((dst) - (src)) >> 2) - 2) & 0xFFFFFF))

setupExceptionVectors:
	adr r0, _vectorStubs
	mov r1, #A9_VECTORS_START
	ldmia r0!, {r2-r9}
	stmia r1!, {r2-r9}
	ldm r0, {r2-r5}
	stm r1, {r2-r5}
	bx lr
_vectorStubs:
	ldr pc, irqHandlerPtr
	irqHandlerPtr:                  .word irqHandler
	ldr pc, fiqHandlerPtr
	fiqHandlerPtr:                  .word (A9_VECTORS_START + 0x08)
	ldr pc, svcHandlerPtr
	svcHandlerPtr:                  .word (A9_VECTORS_START + 0x10)
	ldr pc, undefInstrHandlerPtr
	undefInstrHandlerPtr:           .word undefInstrHandler
	ldr pc, prefetchAbortHandlerPtr
	prefetchAbortHandlerPtr:        .word prefetchAbortHandler
	ldr pc, dataAbortHandlerPtr
	dataAbortHandlerPtr:            .word dataAbortHandler


setupTcms:
	ldr r0, =(DTCM_BASE | 0x0A) @ Base = 0xFFF00000, size = 16 KB
	mov r1, #(ITCM_BASE | 0x24) @ Base = 0x00000000, size = 512 KB (32 KB mirrored)
	mcr p15, 0, r0, c9, c1, 0   @ Write DTCM region reg
	mcr p15, 0, r1, c9, c1, 1   @ Write ITCM region reg
	mrc p15, 0, r0, c1, c0, 0   @ Read control register
	orr r0, r0, #0x50000        @ Enable DTCM and ITCM
	mcr p15, 0, r0, c1, c0, 0   @ Write control register
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


#define REGION_4KB   (0b01011)
#define REGION_8KB   (0b01100)
#define REGION_16KB  (0b01101)
#define REGION_32KB  (0b01110)
#define REGION_64KB  (0b01111)
#define REGION_128KB (0b10000)
#define REGION_256KB (0b10001)
#define REGION_512KB (0b10010)
#define REGION_1MB   (0b10011)
#define REGION_2MB   (0b10100)
#define REGION_4MB   (0b10101)
#define REGION_8MB   (0b10110)
#define REGION_16MB  (0b10111)
#define REGION_32MB  (0b11000)
#define REGION_64MB  (0b11001)
#define REGION_128MB (0b11010)
#define REGION_256MB (0b11011)
#define REGION_512MB (0b11100)
#define REGION_1GB   (0b11101)
#define REGION_2GB   (0b11110)
#define REGION_4GB   (0b11111)
#define MAKE_REGION(adr, size) ((adr) | ((size)<<1) | 1)

#define PER_NO_ACC             (0)
#define PER_PRIV_RW_USR_NO_ACC (0b0001)
#define PER_PRIV_RW_USR_RO     (0b0010)
#define PER_PRIV_RW_USR_RW     (0b0011)
#define PER_PRIV_RO_USR_NO_ACC (0b0101)
#define PER_PRIV_RO_USR_RO     (0b0110)
#define MAKE_PERMISSIONS(r0, r1, r2, r3, r4, r5, r6, r7) \
        ((r0) | (r1<<4) | (r2<<8) | (r3<<12) | (r4<<16) | (r5<<20) | (r6<<24) | (r7<<28))

setupMpu:
	@ Region 0: ITCM kernel mirror 32 KB
	@ Region 1: ARM9 internal mem + N3DS extension 2 MB
	@ Region 2: IO region 2 MB covers only ARM9 accessible regs
	@ Region 3: VRAM 8 MB
	@ Region 4: DSP mem and AXIWRAM 1 MB
	@ Region 5: DTCM 16 KB
	@ Region 6: Exception vectors + ARM9 bootrom 64 KB
	@ Region 7: - (reserved)
	ldr r0, =MAKE_REGION(ITCM_KERNEL_MIRROR, REGION_32KB)
	mcr p15, 0, r0, c6, c0, 0
	ldr r1, =MAKE_REGION(A9_RAM_BASE,        REGION_2MB)
	mcr p15, 0, r1, c6, c1, 0
	ldr r2, =MAKE_REGION(IO_MEM_ARM9_ONLY,   REGION_2MB)
	mcr p15, 0, r2, c6, c2, 0
	ldr r3, =MAKE_REGION(VRAM_BASE,          REGION_8MB)
	mcr p15, 0, r3, c6, c3, 0
	ldr r0, =MAKE_REGION(DSP_MEM_BASE,       REGION_1MB)
	mcr p15, 0, r0, c6, c4, 0
	ldr r1, =MAKE_REGION(DTCM_BASE,          REGION_16KB)
	mcr p15, 0, r1, c6, c5, 0
	ldr r2, =MAKE_REGION(BOOT9_BASE,         REGION_64KB)
	mcr p15, 0, r2, c6, c6, 0
	mov r3, #0
	mcr p15, 0, r3, c6, c7, 0

	@ Data access permissions:
	@ Region 0: User = --, Privileged = RW
	@ Region 1: User = --, Privileged = RW
	@ Region 2: User = --, Privileged = RW
	@ Region 3: User = --, Privileged = RW
	@ Region 4: User = --, Privileged = RW
	@ Region 5: User = --, Privileged = RW
	@ Region 6: User = --, Privileged = RO
	@ Region 7: User = --, Privileged = --
	ldr r0, =MAKE_PERMISSIONS(PER_PRIV_RW_USR_NO_ACC, PER_PRIV_RW_USR_NO_ACC,
                              PER_PRIV_RW_USR_NO_ACC, PER_PRIV_RW_USR_NO_ACC,
                              PER_PRIV_RW_USR_NO_ACC, PER_PRIV_RW_USR_NO_ACC,
                              PER_PRIV_RO_USR_NO_ACC, PER_NO_ACC)
	mcr p15, 0, r0, c5, c0, 2   @ Write data access permissions

	@ Instruction access permissions:
	@ Region 0: User = --, Privileged = RO
	@ Region 1: User = --, Privileged = RO
	@ Region 2: User = --, Privileged = --
	@ Region 3: User = --, Privileged = --
	@ Region 4: User = --, Privileged = --
	@ Region 5: User = --, Privileged = --
	@ Region 6: User = --, Privileged = RO
	@ Region 7: User = --, Privileged = --
	ldr r1, =MAKE_PERMISSIONS(PER_PRIV_RO_USR_NO_ACC, PER_PRIV_RO_USR_NO_ACC,
                              PER_NO_ACC,             PER_NO_ACC,
                              PER_NO_ACC,             PER_NO_ACC,
                              PER_PRIV_RO_USR_NO_ACC, PER_NO_ACC)
	mcr p15, 0, r1, c5, c0, 3   @ Write instruction access permissions

	@ Data cachable bits:
	@ Region 0 = no
	@ Region 1 = yes
	@ Region 2 = no  <-- Never cache IO regs
	@ Region 3 = yes
	@ Region 4 = yes
	@ Region 5 = no
	@ Region 6 = yes
	@ Region 7 = no
	mov r0, #0b01011010
	mcr p15, 0, r0, c2, c0, 0   @ Data cachable bits

	@ Instruction cachable bits:
	@ Region 0 = no
	@ Region 1 = yes
	@ Region 2 = no
	@ Region 3 = no
	@ Region 4 = no
	@ Region 5 = no
	@ Region 6 = yes
	@ Region 7 = no
	mov r1, #0b01000010
	mcr p15, 0, r1, c2, c0, 1   @ Instruction cachable bits

	@ Write bufferable bits:
	@ Region 0 = no
	@ Region 1 = yes
	@ Region 2 = no  <-- Never buffer IO regs
	@ Region 3 = yes
	@ Region 4 = yes
	@ Region 5 = no
	@ Region 6 = yes
	@ Region 7 = no
	mov r2, #0b01011010
	mcr p15, 0, r2, c3, c0, 0   @ Write bufferable bits

	mrc p15, 0, r0, c1, c0, 0   @ Read control register
	ldr r1, =0x1005             @ MPU, D-Cache and I-Cache bitmask
	orr r0, r0, r1              @ Enable MPU, D-Cache and I-Cache
	mcr p15, 0, r0, c1, c0, 0   @ Write control register
	bx lr


@ Needed by libc
_init:
	bx lr


deinitCpu:
	mov r3, lr

	msr cpsr_cxsf, #0xDF        @ System mode
	@ Stub vectors to endless loops
	mov r0, #A9_RAM_BASE
	mov r1, #6
	ldr r2, =MAKE_BRANCH(0, 0)  @ Endless loop
	deinitCpu_lp:
		str r2, [r0], #8
		subs r1, r1, #1
		bne deinitCpu_lp

	bl flushDCache
	mov r2, #0
	mrc p15, 0, r0, c1, c0, 0   @ Read control register
	ldr r1, =0x1005             @ MPU, D-Cache and I-Cache bitmask
	bic r0, r0, r1              @ Disable MPU, D-Cache and I-Cache
	mcr p15, 0, r0, c1, c0, 0   @ Write control register
	mcr p15, 0, r2, c7, c5, 0   @ Invalidate I-Cache
	mcr p15, 0, r2, c7, c6, 0   @ Invalidate D-Cache
	mcr p15, 0, r2, c7, c10, 4  @ Drain write buffer
	bx r3


.pool
