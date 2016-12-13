.arm
.cpu mpcore
.fpu vfpv2

.global _start
.global clearMem
.global _init

.type vectors STT_FUNC
.type _start STT_FUNC
.type clearMem STT_FUNC
.type _init STT_FUNC

.extern initCpu
.extern __bss_start__
.extern __bss_end__
.extern __libc_init_array
.extern main
.extern deinitCpu
.extern firm_launch

.section ".crt0"



_vectors:
	ldr pc, =_start            @ Reset vector
	ldr pc, =(_vectors + 0x04) @ Undefined instruction vector
	ldr pc, =(_vectors + 0x08) @ Software interrupt (SVC) vector
	ldr pc, =(_vectors + 0x0C) @ Prefetch abort vector
	ldr pc, =(_vectors + 0x10) @ Data abort vector
	ldr pc, =(_vectors + 0x14) @ Reserved (unused) vector
	ldr pc, =(_vectors + 0x18) @ Interrupt (IRQ) vector
	ldr pc, =(_vectors + 0x1C) @ Fast interrupt (FIQ) vector
.pool


_start:
	cpsid aif, #0x1F           @ Disable all interrupts, system mode

	bl initCpu

	@ Clear bss section
	ldr r0, =__bss_start__
	ldr r1, =__bss_end__
	sub r1, r1, r0
	bl clearMem

	blx __libc_init_array      @ Initialize ctors and dtors

	mov r0, #0                 @ argc
	mov r1, #0                 @ argv
	blx main
	cmp r0, #0
	bne .
	bl deinitCpu
	b firm_launch
.pool


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


_init:
	bx lr
