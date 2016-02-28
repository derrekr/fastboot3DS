#include "mem_map.h"

.arm
.arch armv5te
.fpu softvfp

.global _start
.global _init
.global flushDCache
.global invalidateDCache
.global invalidateICache

.type _start STT_FUNC
.type _init STT_FUNC
.type setupMpu STT_FUNC
.type flushDCache STT_FUNC
.type invalidateDCache STT_FUNC
.type invalidateICache STT_FUNC
.type drainWriteBuffer STT_FUNC 

.section .init

_start:
	mrs r0, cpsr
	orr r0, r0, #0xC0
	msr cpsr_c, r0

	ldr sp, =(0x080FFE00-8)

	ldr r0, =0x1FFFFFF0
	mov r1, #0
	str r1, [r0, #0xC]         @ Clear arm9 communication fields
	str r1, [r0, #0x8]

	bl bss_clear
	blx heap_init
	blx __libc_init_array

	//bl setupMpu                @ Disabled for now
	
	blx main
	cmp r0, #-1
	beq endlessLoop

	//mrc p15, 0, r1, c1, c0, 0  @ Read control register
	//ldr r2, =0x1005            @ MPU, D-Cache and I-Cache bitmask
	//bic r1, r1, r2             @ Disable MPU, D-Cache and I-Cache
	//mcr p15, 0, r1, c1, c0, 0  @ Write control register
	//mov r1, #0
	//mcr p15, 0, r1, c7, c10, 4 @ Drain write buffer

	blx firm_launch

endlessLoop:
	mov r0, #0
	mcr p15, 0, r0, c7, c0, 4    @ Wait for interrupt
	b endlessLoop


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


#define REGION_4KB   (0b01011)
#define REGION_8KB   (0b01100)
#define REGION_16KB  (0b01101)
#define REGION_32KB  (0b01110)
#define REGION_64KB  (0b01111)
#define REGION_128KB (0b10000)
#define REGION_256KB (0b10001)
#define REGION_512KB (0b10010)
#define REGION_1MB   (0b10011)
#define REGION_2MB   (0b10100)
#define REGION_4MB   (0b10101)
#define REGION_8MB   (0b10110)
#define REGION_16MB  (0b10111)
#define REGION_32MB  (0b11000)
#define REGION_64MB  (0b11001)
#define REGION_128MB (0b11010)
#define REGION_256MB (0b11011)
#define REGION_512MB (0b11100)
#define REGION_1GB   (0b11101)
#define REGION_2GB   (0b11110)
#define REGION_4GB   (0b11111)
#define MAKE_REGION(adr, size) ((adr) | ((size)<<1) | 1)

#define PER_NO_ACC             (0)
#define PER_PRIV_RW_USR_NO_ACC (0b0001)
#define PER_PRIV_RW_USR_RO     (0b0010)
#define PER_PRIV_RW_USR_RW     (0b0011)
#define PER_PRIV_RO_USR_NO_ACC (0b0101)
#define PER_PRIV_RO_USR_RO     (0b0110)
#define MAKE_PERMISSIONS(_0, _1, _2, _3, _4, _5, _6, _7) ((_0) | (_1<<4) | (_2<<8) | (_3<<12) | (_4<<16) | (_5<<20) | (_6<<24) | (_7<<28))

setupMpu:
	mov r2, lr
	bl invalidateICache
	bl invalidateDCache
	bl drainWriteBuffer
	mov r0, #MAKE_REGION(0x00000000, REGION_4GB)
	mcr p15, 0, r0, c6, c0, 0                      @ Region 0: Background 4GB
	ldr r0, =MAKE_REGION(0x00000000, REGION_128MB)
	mcr p15, 0, r0, c6, c1, 0                      @ Region 1: ITCM + mirrors 128 MB
	ldr r0, =MAKE_REGION(0x08000000, REGION_2MB)
	mcr p15, 0, r0, c6, c2, 0                      @ Region 2: ARM9 mem
	ldr r0, =MAKE_REGION(0x10000000, REGION_128MB)
	mcr p15, 0, r0, c6, c3, 0                      @ Region 3: IO region
	ldr r0, =MAKE_REGION(0x18000000, REGION_128MB)
	mcr p15, 0, r0, c6, c4, 0                      @ Region 4: VRAM + DSP
	ldr r0, =MAKE_REGION(0x1FF80000, REGION_512KB)
	mcr p15, 0, r0, c6, c5, 0                      @ Region 5: AXIWRAM
	ldr r0, =MAKE_REGION(0x20000000, REGION_256MB)
	mcr p15, 0, r0, c6, c6, 0                      @ Region 6: FCRAM
	ldr r0, =MAKE_REGION(0xFFF00000, REGION_1MB)
	mcr p15, 0, r0, c6, c7, 0                      @ Region 7: DTCM + bootrom
	@ Region 0 no access at all. Region 7 user and privileged read-only.
	@ All others rw access for user and privileged.
	ldr r0, =MAKE_PERMISSIONS(PER_NO_ACC, PER_PRIV_RW_USR_RW,
                              PER_PRIV_RW_USR_RW, PER_PRIV_RW_USR_RW,
                              PER_PRIV_RW_USR_RW, PER_PRIV_RW_USR_RW,
                              PER_PRIV_RW_USR_RW, PER_PRIV_RO_USR_RO)
	mcr p15, 0, r0, c5, c0, 2                      @ Data access permissions
	mcr p15, 0, r0, c5, c0, 3                      @ Instruction access permissions
	mov r0, #0b11010010                            @ Data cachable bits:
                                                   @ Region 0 = no
                                                   @ Region 1 = yes
                                                   @ Region 2 = no  <-- If yes this messes up everything
                                                   @ Region 3 = no  <-- Never cache IO regs
                                                   @ Region 4 = yes
                                                   @ Region 5 = no
                                                   @ Region 6 = yes
                                                   @ Region 7 = yes
	mcr p15, 0, r0, c2, c0, 0                      @ Data cachable bits
	mov r0, #0b10100110                            @ Instruction cachable bits:
                                                   @ Region 0 = no
                                                   @ Region 1 = yes
                                                   @ Region 2 = yes
                                                   @ Region 3 = no
                                                   @ Region 4 = no
                                                   @ Region 5 = yes
                                                   @ Region 6 = no
                                                   @ Region 7 = yes
	mcr p15, 0, r0, c2, c0, 1                      @ Instruction cachable bits
	mov r0, #0b01010010                            @ Write bufferable bits:
                                                   @ Region 0 = no
                                                   @ Region 1 = yes
                                                   @ Region 2 = no  <-- If yes this messes up everything
                                                   @ Region 3 = no
                                                   @ Region 4 = yes
                                                   @ Region 5 = no
                                                   @ Region 6 = yes
                                                   @ Region 7 = no
	mcr p15, 0, r0, c3, c0, 0                      @ Write bufferable bits
	mrc p15, 0, r0, c1, c0, 0                      @ Read control register
	ldr r1, =0x1005                                @ MPU, D-Cache and I-Cache bitmask
	orr r0, r0, r1                                 @ Enable MPU, D-Cache and I-Cache
	mcr p15, 0, r0, c1, c0, 0                      @ Write control register
	bx r2


#define ICACHE_SIZE	0x2000
#define DCACHE_SIZE	0x1000
#define CACHE_LINE_SIZE	32

flushDCache:                @ This is code from libnds which is from the ARM docs
	mov	r1, #0
outer_loop:
	mov	r0, #0
inner_loop:
	orr	r2, r1, r0			@ generate segment and line address
	mcr	p15, 0, r2, c7, c14, 2		@ clean and flush the line
	add	r0, r0, #CACHE_LINE_SIZE
	cmp	r0, #DCACHE_SIZE/4
	bne	inner_loop
	add	r1, r1, #0x40000000
	cmp	r1, #0
	bne	outer_loop
	b	drainWriteBuffer

invalidateDCache:
	mov r0, #0
	mcr p15, 0, r0, c7, c6, 0  @ "Flush" data cache
	bx lr

invalidateICache:
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0  @ "Flush" instruction cache
	bx lr

@ Waits until cache is empty
drainWriteBuffer:
	mov r0, #0
	mcr p15, 0, r0, c7, c10, 4 @ Drain write buffer
	bx lr
	
.pool

// needed by libc
_init:
	bx lr
