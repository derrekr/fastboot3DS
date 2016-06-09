.arm
.arch armv5te
.fpu softvfp

.global undefHandler
.global prefetchAbortHandler
.global dataAbortHandler

.type undefHandler STT_FUNC
.type prefetchAbortHandler STT_FUNC
.type dataAbortHandler STT_FUNC
.type exceptionHandler STT_FUNC

.extern guruMeditation



undefHandler:
	ldr sp, =(0x02000000-8)
	stmfd sp!, {r0-r12, lr}     @ Save all non-banked regs and lr (pc)
	mov r0, #0
	b exceptionHandler
prefetchAbortHandler:
	ldr sp, =(0x02000000-8)
	stmfd sp!, {r0-r12, lr}
	mov r0, #1
	b exceptionHandler
dataAbortHandler:
	ldr sp, =(0x02000000-8)
	stmfd sp!, {r0-r12, lr}
	mov r0, #2
exceptionHandler:
	mrs r1, spsr                @ Get saved cpsr
	str r1, [sp, #-4]!          @ Save spsr on exception stack
	mov r1, sp
	msr cpsr_c, #0xDF           @ Disable all interrupts, system mode
	str lr, [r1, #-4]!          @ Save lr on exception stack
	mov r2, sp
	mov sp, r1
	b guruMeditation            @ r0 = exception type, r1 = reg dump ptr {lr, cpsr, r0-r12, pc + X}, r2 = sp
