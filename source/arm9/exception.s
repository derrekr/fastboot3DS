#include "asmfunc.h"
#include "mem_map.h"

.arm
.cpu arm946e-s
.fpu softvfp

.extern deinitCpu
.extern guruMeditation
.extern irqHandlerTable



ASM_FUNC undefInstrHandler
	msr cpsr_f, #(0<<29)        @ Abuse conditional flags in cpsr for temporary exception type storage
	b exceptionHandler
ASM_FUNC prefetchAbortHandler
	msr cpsr_f, #(1<<29)
	b exceptionHandler
ASM_FUNC dataAbortHandler
	msr cpsr_f, #(2<<29)
ASM_FUNC exceptionHandler
	sub sp, #68
	stmia sp, {r0-r14}^            @ Save all user/system mode regs except pc
	mrs r2, spsr                   @ Get saved cpsr
	mrs r3, cpsr
	lsr r0, r3, #29                @ Get back the exception type from cpsr
	and r1, r2, #0x1F
	cmp r1, #0x10                  @ User mode
	beq exceptionHandler_skip_other_mode
	add r4, sp, #32
	msr cpsr_c, r2
	stmia r4!, {r8-r14}            @ Some regs are written twice but we don't care
	msr cpsr_c, r3
exceptionHandler_skip_other_mode:
	str lr, [sp, #60]              @ Save lr (pc) on exception stack
	str r2, [sp, #64]              @ Save spsr (cpsr) on exception stack
	mov r4, r0
	mov r5, sp
	msr cpsr_cxsf, #0xDF           @ Disable all interrupts, system mode
	bl deinitCpu
	mov r0, r4
	mov sp, r5
	mov r1, r5
	b guruMeditation               @ r0 = exception type, r1 = reg dump ptr {r0-r14, pc (unmodified), cpsr}


ASM_FUNC irqHandler
	stmfd sp!, {r0-r3, r12, lr}
	ldr r12, =(IO_MEM_ARM9_ONLY + 0x1000) @ REG_IRQ_IE
	ldm r12, {r1, r2}
	and r1, r1, r2
	mov r3, #0x80000000
	irqHandler_find_first_lp:
		clz r0, r1
		bics r1, r1, r3, lsr r0
		bne irqHandler_find_first_lp
	mov r1, r3, lsr r0
	str r1, [r12, #4]           @ REG_IRQ_IF
	rsb r2, r0, #31             @ r2 = 31 - r0
	ldr r1, =irqHandlerTable
	ldr r0, [r1, r2, lsl #2]
	mrs r2, spsr
	str r2, [sp, #-4]!
	msr cpsr_c, #0x5F           @ Interrupts enabled, system mode
	str lr, [sp, #-4]!
	cmp r0, #0
	blxne r0
	ldr lr, [sp], #4
	msr cpsr_c, #0xD2           @ Interrupts disabled, IRQ mode
	ldr r2, [sp], #4
	msr spsr, r2
	ldmfd sp!, {r0-r3, r12, lr}
	subs pc, lr, #4
