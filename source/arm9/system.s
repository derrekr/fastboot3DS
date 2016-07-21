#include "mem_map.h"

.arm
.arch armv5te
.fpu softvfp

.global initSystem
.global finiSystem

.type initSystem STT_FUNC
.type resetCriticalHardware STT_FUNC
.type setupExceptionVectors STT_FUNC
.type setupTcms STT_FUNC
.type setupMpu STT_FUNC
.type finiSystem STT_FUNC

.extern undefHandler
.extern prefetchAbortHandler
.extern dataAbortHandler
.extern flushDCache

.section ".init"



initSystem:
	mov r10, lr

	@ Control register:
	@ [19] ITCM load mode         : disabled
	@ [18] ITCM                   : disabled
	@ [17] DTCM load mode         : disabled
	@ [16] DTCM                   : disabled
	@ [15] Disable loading TBIT   : disabled
	@ [14] Round-robin replacement: disabled
	@ [13] Vector select          : 0xFFFF0000
	@ [12] I-Cache                : disabled
	@ [7]  Endianess              : little
	@ [2]  D-Cache                : disabled
	@ [0]  MPU                    : disabled
	ldr r0, =0x2078
	mcr p15, 0, r0, c1, c0, 0   @ Write control register

	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0   @ Invalidate I-Cache
	mcr p15, 0, r0, c7, c6, 0   @ Invalidate D-Cache
	mcr p15, 0, r0, c7, c10, 4  @ Drain write buffer

	bl resetCriticalHardware
	bl setupExceptionVectors    @ Setup the vectors in ARM9 mem bootrom vectors jump to
	bl setupTcms                @ Setup and enable DTCM and ITCM
	bl setupMpu
	bx r10


// Based on code compiled with gcc
resetCriticalHardware:
	mov r3, #0
	mov r12, #0xFFFFFFFF
	ldr r2, =(IO_MEM_ARM9_ONLY + 0x2000) @ NDMA regs
	ldr r1, =(IO_MEM_ARM9_ONLY + 0x3000) @ Timer regs
	ldr r0, =(IO_MEM_ARM9_ONLY + 0x1000) @ IRQ regs
	stmia r0, {r3, r12}                  @ Disable and aknowledge all interrupts

	str r3, [r2], #0x1C                  @ REG_NDMA_GLOBAL_CNT
	mov r12, #8
	loop_disable:
		str r3, [r2], #0x1C              @ REG_NDMA_CNT(n) = 0
		subs r12, r12, #1
		bne loop_disable

	strh r3, [r1, #2]                    @ REG_TIMER0_CNT
	strh r3, [r1, #6]                    @ REG_TIMER1_CNT
	strh r3, [r1, #0xA]                  @ REG_TIMER2_CNT
	strh r3, [r1, #0xE]                  @ REG_TIMER3_CNT
	bx lr


#define MAKE_BRANCH(src, dst) (0xEA000000 | (((((dst) - (src)) >> 2) - 2) & 0xFFFFFF))

setupExceptionVectors:
	ldr r0, =_vectorStubs_
	mov r1, #A9_RAM_BASE
	ldmia r0!, {r2-r9}
	stmia r1!, {r2-r9}
	ldmia r0, {r2-r5}
	stmia r1, {r2-r5}
	bx lr
_vectorStubs_:
	.word MAKE_BRANCH(A9_RAM_BASE + 0x00, A9_RAM_BASE + 0x00) // IRQ
	.word 0
	.word MAKE_BRANCH(A9_RAM_BASE + 0x08, A9_RAM_BASE + 0x08) // FIQ
	.word 0
	.word MAKE_BRANCH(A9_RAM_BASE + 0x10, A9_RAM_BASE + 0x10) // SVC
	.word 0
	ldr pc, undefHandlerPtr
	undefHandlerPtr:                .word undefHandler
	ldr pc, prefetchAbortHandlerPtr
	prefetchAbortHandlerPtr:        .word prefetchAbortHandler
	ldr pc, dataAbortHandlerPtr
	dataAbortHandlerPtr:            .word dataAbortHandler


setupTcms:
	ldr r0, =(DTCM_BASE | 0x0A) @ Base = 0xFFF00000, size = 16 KB
	mcr p15, 0, r0, c9, c1, 0   @ Write DTCM region reg
	mov r0, #(ITCM_BASE | 0x24) @ Base = 0x00000000, size = 512 KB (32 KB mirrored)
	mcr p15, 0, r0, c9, c1, 1   @ Write ITCM region reg

	mrc p15, 0, r0, c1, c0, 0   @ Read control register
	orr r0, r0, #0x50000        @ Enable DTCM and ITCM
	mcr p15, 0, r0, c1, c0, 0   @ Write control register
	bx lr


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
#define MAKE_PERMISSIONS(r0, r1, r2, r3, r4, r5, r6, r7) \
	((r0) | (r1<<4) | (r2<<8) | (r3<<12) | (r4<<16) | (r5<<20) | (r6<<24) | (r7<<28))

setupMpu:
	@ Region 0: ITCM kernel mirror 32 KB
	@ Region 1: ARM9 internal mem 2 MB covers N3DS extension if we want to load code there
	@ Region 2: IO region 2 MB covers only ARM9 accessible regs
	@ Region 3: VRAM 4 MB
	@ Region 4: AXIWRAM 512 KB
	@ Region 5: FCRAM 128 MB
	@ Region 6: DTCM 16 KB
	@ Region 7: Exception vectors + ARM9 bootrom 32 KB
	ldr r0, =MAKE_REGION(ITCM_KERNEL_MIRROR, REGION_32KB)
	mcr p15, 0, r0, c6, c0, 0
	ldr r0, =MAKE_REGION(A9_RAM_BASE,        REGION_2MB)
	mcr p15, 0, r0, c6, c1, 0
	ldr r0, =MAKE_REGION(IO_MEM_ARM9_ONLY,   REGION_2MB)
	mcr p15, 0, r0, c6, c2, 0
	ldr r0, =MAKE_REGION(VRAM_BASE,          REGION_4MB)
	mcr p15, 0, r0, c6, c3, 0
	ldr r0, =MAKE_REGION(AXIWRAM_BASE,       REGION_512KB)
	mcr p15, 0, r0, c6, c4, 0
	ldr r0, =MAKE_REGION(FCRAM_BASE,         REGION_128MB)
	mcr p15, 0, r0, c6, c5, 0
	ldr r0, =MAKE_REGION(DTCM_BASE,          REGION_16KB)
	mcr p15, 0, r0, c6, c6, 0
	ldr r0, =MAKE_REGION(BOOT9_BASE,         REGION_32KB)
	mcr p15, 0, r0, c6, c7, 0

	@ Data access permissions:
	@ Region 0: User = --, Privileged = RW
	@ Region 1: User = RW, Privileged = RW
	@ Region 2: User = RW, Privileged = RW
	@ Region 3: User = RW, Privileged = RW
	@ Region 4: User = RO, Privileged = RW
	@ Region 5: User = RW, Privileged = RW
	@ Region 6: User = --, Privileged = RW
	@ Region 7: User = RO, Privileged = RO
	ldr r0, =MAKE_PERMISSIONS(PER_PRIV_RW_USR_NO_ACC, PER_PRIV_RW_USR_RW,
                              PER_PRIV_RW_USR_RW,     PER_PRIV_RW_USR_RW,
                              PER_PRIV_RW_USR_RO,     PER_PRIV_RW_USR_RW,
                              PER_PRIV_RW_USR_NO_ACC, PER_PRIV_RO_USR_RO)
	mcr p15, 0, r0, c5, c0, 2   @ Data access permissions

	@ Instruction access permissions:
	@ Region 0: User = --, Privileged = RO
	@ Region 1: User = RO, Privileged = RO
	@ Region 2: User = --, Privileged = --
	@ Region 3: User = --, Privileged = --
	@ Region 4: User = --, Privileged = RO
	@ Region 5: User = RO, Privileged = RO
	@ Region 6: User = --, Privileged = --
	@ Region 7: User = RO, Privileged = RO
	ldr r0, =MAKE_PERMISSIONS(PER_PRIV_RO_USR_NO_ACC, PER_PRIV_RO_USR_RO,
                              PER_NO_ACC,             PER_NO_ACC,
                              PER_PRIV_RO_USR_NO_ACC, PER_PRIV_RO_USR_RO,
                              PER_NO_ACC,             PER_PRIV_RO_USR_RO)
	mcr p15, 0, r0, c5, c0, 3   @ Instruction access permissions

	@ Data cachable bits:
	@ Region 0 = no
	@ Region 1 = yes
	@ Region 2 = no  <-- Never cache IO regs
	@ Region 3 = yes
	@ Region 4 = yes
	@ Region 5 = yes
	@ Region 6 = no
	@ Region 7 = no
	mov r0, #0b00111010
	mcr p15, 0, r0, c2, c0, 0   @ Data cachable bits

	@ Instruction cachable bits:
	@ Region 0 = no
	@ Region 1 = yes
	@ Region 2 = no
	@ Region 3 = no
	@ Region 4 = yes
	@ Region 5 = yes
	@ Region 6 = no
	@ Region 7 = yes
	mov r0, #0b10110010
	mcr p15, 0, r0, c2, c0, 1   @ Instruction cachable bits

	@ Write bufferable bits:
	@ Region 0 = no
	@ Region 1 = yes
	@ Region 2 = no  <-- Never buffer IO reg writes
	@ Region 3 = yes
	@ Region 4 = yes
	@ Region 5 = yes
	@ Region 6 = no
	@ Region 7 = no
	mov r0, #0b00111010
	mcr p15, 0, r0, c3, c0, 0   @ Write bufferable bits

	mrc p15, 0, r0, c1, c0, 0   @ Read control register
	ldr r1, =0x1005             @ MPU, D-Cache and I-Cache bitmask
	orr r0, r0, r1              @ Enable MPU, D-Cache and I-Cache
	mcr p15, 0, r0, c1, c0, 0   @ Write control register
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0   @ Invalidate I-Cache
	mcr p15, 0, r0, c7, c6, 0   @ Invalidate D-Cache
	bx lr


finiSystem:
	mov r4, lr
	bl resetCriticalHardware

	@ Stub vectors to endless loops
	mov r0, #A9_RAM_BASE
	mov r1, #6
	ldr r2, =MAKE_BRANCH(0, 0)  @ Endless loop
	loop_stub:
		str r2, [r0], #8
		subs r1, r1, #1
		bne loop_stub

	bl flushDCache
	mrc p15, 0, r0, c1, c0, 0   @ Read control register
	ldr r1, =0x1005             @ MPU, D-Cache and I-Cache bitmask
	bic r0, r0, r1              @ Disable MPU, D-Cache and I-Cache
	mcr p15, 0, r0, c1, c0, 0   @ Write control register
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0   @ Invalidate I-Cache
	mcr p15, 0, r0, c7, c6, 0   @ Invalidate D-Cache
	bx r4
