#include "mem_map.h"
#include "version.h"

.arm
.arch armv5te
.fpu softvfp

.global _start
.global clearMem
.global _init
.global firmLaunchEntry9
.global firmLaunchEntry11

.type _start STT_FUNC
.type clearMem STT_FUNC
.type _init STT_FUNC

.extern initSystem
.extern finiSystem
.extern __libc_init_array
.extern initHardware
.extern main
.extern firm_launch
.extern __bss_start__
.extern __bss_end__
.extern __end__
.extern fake_heap_start
.extern fake_heap_end

.section ".crt0"



_start:
	msr cpsr_c, #0xDF           @ Disable all interrupts, system mode
	b skip_pool

	.string "3DS BOOTLOADER "
	.word   BOOTLOADER_VERSION
	.word   0                   @ Flags
firmLaunchEntry9:
	.word   0                   @ Entrypoint override field
firmLaunchEntry11:
	.word   0

skip_pool:
	ldr sp, =A9_STACK_END

	bl initSystem

	@ Clear bss section
	ldr r0, =__bss_start__
	ldr r1, =__bss_end__
	bl clearMem

	@ Setup newlib heap
	ldr r0, =__end__
	add r0, r0, #4              @ Do not overwrite the value at symbol __end__
	ldr r1, =fake_heap_start
	str r0, [r1]
	add r0, r0, #A9_HEAP_SIZE
	ldr r1, =fake_heap_end
	str r0, [r1]

	blx __libc_init_array       @ Initialize ctors and dtors
	blx initHardware
	mov r0, #0
	mov r1, #0
	blx main
	cmp r0, #0
	bne .
	bl finiSystem
	b firm_launch


@ void clearMem(u32 *start, u32 *end)
clearMem:
	sub r1, r1, r0
	mov r2, #0
	loop_clear:
		str r2, [r0], #4
		subs r1, r1, #4
		bne loop_clear
	bx lr


@ needed by libc
_init:
	bx lr
