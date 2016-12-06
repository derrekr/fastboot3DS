#include "mem_map.h"
#include "version.h"

.arm
.cpu arm946e-s
.fpu softvfp

.global _start
.global clearMem
.global _init

.type _start STT_FUNC
.type clearMem STT_FUNC
.type _init STT_FUNC

.extern initCpu
.extern deinitCpu
.extern __libc_init_array
.extern main
.extern firm_launch
.extern __bss_start__
.extern __bss_end__
.extern __end__
.extern fake_heap_start
.extern fake_heap_end

.section ".crt0"



_start:
	msr cpsr_cxsf, #0xDF           @ Disable all interrupts, system mode
	b skip_pool

	.string "3DS BOOTLOADER "
	.word   BOOTLOADER_VERSION

skip_pool:
	bl initCpu

	@ Clear bss section
	ldr r0, =__bss_start__
	ldr r1, =__bss_end__
	sub r1, r1, r0
	bl clearMem

	@ Setup newlib heap
	ldr r0, =(__end__ + 12)     @ Do not overwrite the value at symbol __end__
	bic r0, r0, #7              @ Align to 8 bytes
	ldr r1, =fake_heap_start
	str r0, [r1]
	mov r0, #A9_HEAP_END
	ldr r1, =fake_heap_end
	str r0, [r1]

	blx __libc_init_array       @ Initialize ctors and dtors

	mov r0, #0                  @ argc
	mov r1, #0                  @ argv
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


@ Needed by libc
_init:
	bx lr
