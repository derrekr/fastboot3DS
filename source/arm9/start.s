.arm

.arch armv5te
.fpu softvfp

.global _start
.global _init

.section .init

_start:

	mrs r0, cpsr
	orr r0, r0, #0xC0
	msr cpsr_c, r0

	ldr sp, =(0x080FFE00-8)

	ldr r0, =0x1FFFFFFC
	ldr r1, =0x1FFFFFF8
	mov r2, #0
	str r2, [r0]	// clear arm9 communication fields
	str r2, [r1]

	bl bss_clear

	bl heap_init

	bl	__libc_init_array
	
	blx main
	b .
	

bss_clear:
	ldr r1, =__bss_start__
	ldr r2, =__bss_end__
	mov r3, #0

	loop_clear:
	cmp r1, r2
	bxeq lr
	strb r3, [r1]
	add r1, r1, #1
	b loop_clear
	
.pool

// needed by libc
_init:
	bx lr
