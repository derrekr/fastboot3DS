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

#include "asmfunc.h"

.arm
.cpu mpcore
.fpu vfpv2

.extern deinitCpu
.extern privIrqHandlerTable
.extern irqHandlerTable



ASM_FUNC tmpExceptionHandler
	bl deinitCpu
	ldr r0, =0x10202A04
	mov r1, #0xFF
	orr r1, #0x1000000
	str r1, [r0]
tmpExceptionHandler_lp:
	wfi
	b tmpExceptionHandler_lp
.pool


ASM_FUNC irqHandler
	sub lr, lr, #4
	srsfd sp!, #31               @ Store lr and spsr on system mode stack
	cps #31                      @ Switch to system mode
	stmfd sp!, {r0-r3, r12, lr}
	mov r12, #0x17000000
	orr r12, r12, #0xE00000
	ldr r0, [r12, #0x10C]        @ REG_CPU_II_AKN
	and r1, r0, #0x7F
	cmp r1, #32
	ldrlo r2, =privIrqHandlerTable
	mrclo p15, 0, r3, c0, c0, 5  @ Get CPU ID
	andlo r3, r3, #3
	addlo r2, r2, r3, lsl #7
	ldrhs r2, =irqHandlerTable
	subhs r1, r1, #32
	ldr r3, [r2, r1, lsl #2]
	cmp r3, #0
	beq irqHandler_skip_processing
	stmfd sp!, {r0, r12}
	cpsie i
	blx r3
	cpsid i
	ldmfd sp!, {r0, r12}
irqHandler_skip_processing:
	str r0, [r12, #0x110]        @ REG_CPU_II_EOI
	ldmfd sp!, {r0-r3, r12, lr}
	rfefd sp!                    @ Restore lr (pc) and spsr (cpsr)
