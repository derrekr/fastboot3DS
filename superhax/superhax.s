/*
 *   This file is part of fastboot 3DS
 *   Copyright (C) 2018 derrek, profi200
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

.arm
.cpu arm946e-s
.fpu softvfp

.global _vectors

.type _vectors %function
.type irqHandlerHook %function
.type hook1 %function
.type hook2 %function

.section ".crt0", "ax"



.skip 372, 0  @ Pad to 512 bytes

_vectors:
	@ These vectors are copied over the existing ones in ARM9 memory
	@ at address 0x08000000. The next IRQ (MMC?) will then branch to
	@ our hook function instead of the regular IRQ handler.
	ldr pc, irqFuncPtr
	irqFuncPtr:         .word irqHandlerHook
	b .
	.word 0
	b .
	.word 0
	b .
	.word 0
	b .
	.word 0
	b .
	.word 0


irqHandlerHook:
	@ Overwrites 2 debug function pointers left in boot9 which are
	@ called before boot9 jumps to the FIRM entrypoint. It also
	@ restores the original IRQ vector before jumping to boot9's
	@ IRQ handler. The second function skips the bootrom lock code.
	stmfd sp!, {r0-r5, r12, lr}  @ Prelogue of boot9 IRQ handler (yes, they save too many regs).
	adr r0, hook1
	adr r1, hook2
	ldr r3, =0xFFFF0C78          @ boot9 IRQ handler
	ldr r2, =0xFFF00058          @ DTCM function pointers
	str r3, irqFuncPtr           @ Restore IRQ vector
	strd r0, r1, [r2]            @ Overwrite function pointers
	add pc, r3, #4               @ Branch to real IRQ handler skipping prelogue


hook1:
	@ Overwrites a debug function pointer from boot11 called just
	@ before the final jump to the entrypoint. This is a race
	@ condition because boot1 will overwrite it itself while we are
	@ messing with it. This also copies a tiny function to AXIWRAM start.
	@ This function skips the bootrom lock wait code.
	ldr r0, =0x1FFE802C  @ boot11 function pointer
	ldr r1, =0x1FF80000
	ldr r2, hook2
	str r2, [r1]         @ Copy hook2 to AXIWRAM start
	str r1, [r0]         @ Overwrite pointer
	hook1_lp:            @ Loop until pointer changes (race)
		ldr r2, [r0]
		cmp r2, r1
		beq hook1_lp
	str r1, [r0]         @ Overwrite pointer again
	bx lr


hook2:
	bx r0

.pool
