#include "asmfunc.h"
#include "mem_map.h"

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
	srsfd sp!, #31              @ Store lr and spsr on system mode stack
	cps #31                     @ Switch to system mode
	stmfd sp!, {r0-r3, r12, lr}
	mov r0, #0x17000000
	orr r0, r0, #0xE00000
	ldr r1, [r0, #0x10C]        @ REG_CPU_II_AKN
	str r1, [r0, #0x110]        @ REG_CPU_II_EOI
	and r0, r1, #0x7F
	cmp r0, #16
	ldrlo r2, =privIrqHandlerTable
	addlo r2, r2, r1, lsr #8
	ldrhs r2, =irqHandlerTable
	subhs r0, r0, #16
	ldr r3, [r2, r0, lsl #2]
	cmp r3, #0
	beq irqHandler_skip_processing
	cpsie i
	blx r3
	cpsid i
irqHandler_skip_processing:
	ldmfd sp!, {r0-r3, r12, lr}
	rfefd sp!                   @ Restore lr (pc) and spsr (cpsr)
