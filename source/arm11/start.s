.arm

.cpu mpcore

.global _start
.global _init

.section .init

_start:

	@ Disable all interrupts
	cpsid aif

	@ Invalidate Entire Instruction Cache,
	@ also flushes the branch target cache
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0

	@ Clear and Invalidate Entire Data Cache
	mov r0, #0
	mcr p15, 0, r0, c7, c14, 0

	@ Data Synchronization Barrier
	mov r0, #0
	mcr p15, 0, r0, c7, c10, 4

	@ Disable the MMU and data cache
	@ (the MMU is already disabled)
	mrc p15, 0, r1, c1, c0, 0
	bic r1, r1, #0b101
	mcr p15, 0, r1, c1, c0, 0

	ldr sp, =(0x1FFFFE00-8) // stack starts at the end of AXIWRAM

	ldr r0, =0x1FFFFFFC
	ldr r1, =0x1FFFFFF8
	mov r2, #0
	str r2, [r0]	// clear arm9 communication fields
	str r2, [r1]

	bl bss_clear

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


_init:
	bx lr
