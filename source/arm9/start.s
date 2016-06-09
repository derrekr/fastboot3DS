#include "mem_map.h"
#include "version.h"

.arm
.arch armv5te
.fpu softvfp

.global _start
.global _init
.global firmLaunchEntry9
.global firmLaunchEntry11

.type _start STT_FUNC
.type _init STT_FUNC

.extern setupSystem
.extern __libc_init_array
.extern heap_init
.extern main
.extern firm_launch
.extern __bss_start__
.extern __bss_end__

.section .init

_start:

	b skip_pool

	.string "3DS BOOTLOADER "
	.word   BOOTLOADER_VERSION
	.word   0                   @ Flags
firmLaunchEntry9:
	.word   0                   @ Entrypoint override field
firmLaunchEntry11:
	.word   0

skip_pool:
	msr cpsr_c, #0xDF           @ Disable all interrupts, system mode
	ldr sp, =(A9_STUB_ENTRY)

	bl setupSystem

	ldr r0, =__bss_start__
	ldr r1, =__bss_end__
	sub r1, r1, r0
	mov r2, #0
	loop_bss_clear:
		str r2, [r0], #4
		subs r1, r1, #4
		bne loop_bss_clear

	blx __libc_init_array
	blx heap_init
	blx main
	cmp r0, #0
	beq firm_launch
	b .


@ needed by libc
_init:
	bx lr
