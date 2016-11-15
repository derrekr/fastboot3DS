#include "mem_map.h"

.arm
.arch armv5te
.fpu softvfp

.global undefInstrHandler
.global prefetchAbortHandler
.global dataAbortHandler

.type undefInstrHandler STT_FUNC
.type prefetchAbortHandler STT_FUNC
.type dataAbortHandler STT_FUNC
.type exceptionHandler STT_FUNC

.extern deinitCpu
.extern guruMeditation

.section ".text"



undefInstrHandler:
	msr cpsr_f, #(0<<29)        @ Abuse conditional flags in cpsr for temporary exception type storage
	b exceptionHandler
prefetchAbortHandler:
	msr cpsr_f, #(1<<29)
	b exceptionHandler
dataAbortHandler:
	msr cpsr_f, #(2<<29)
exceptionHandler:
	mov sp, #A9_EXC_STACK_END
	stmfd sp!, {r0-r14}^        @ Save all user/system mode regs except pc
	mrs r5, cpsr
	lsr r5, r5, #29             @ Get back the exception type from cpsr
	mrs r1, spsr                @ Get saved cpsr
	stmfd sp!, {r1, lr}         @ Save spsr and lr (pc) on exception stack
	mov r6, sp
	msr cpsr_c, #0xDF           @ Disable all interrupts, system mode
	bl deinitCpu
	mov r0, r5
	mov sp, r6
	mov r1, r6
	b guruMeditation            @ r0 = exception type, r1 = reg dump ptr {cpsr, pc (unmodified), r0-r14}
