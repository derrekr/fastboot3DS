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
	mov sp, #A9_EXC_STACK_END
	stmfd sp!, {r0-r14}^        @ Save all user/system mode regs except pc
	mrs r4, cpsr
	lsr r4, r4, #29             @ Get back the exception type from cpsr
	mrs r1, spsr                @ Get saved cpsr
	stmfd sp!, {r1, lr}         @ Save spsr and lr (pc) on exception stack
	mov r5, sp
	msr cpsr_cxsf, #0xDF        @ Disable all interrupts, system mode
	bl deinitCpu
	mov r0, r4
	mov sp, r5
	mov r1, r5
	b guruMeditation            @ r0 = exception type, r1 = reg dump ptr {cpsr, pc (unmodified), r0-r14}


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
	cmp r0, #0
	blxne r0
	ldmfd sp!, {r0-r3, r12, lr}
	subs pc, lr, #4
