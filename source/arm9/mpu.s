.arm
.arch armv5te
.fpu softvfp

.global setupMpu
.global disableMpu

.type setupMpu STT_FUNC
.type disableMpu STT_FUNC



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
	@ Region 0: ITCM kernel mirror 32 KB
	@ Region 1: ARM9 internal mem 2 MB covers N3DS extension if we want to load code there
	@ Region 2: IO region 2 MB covers only ARM9 accessible regs
	@ Region 3: VRAM 4 MB
	@ Region 4: AXIWRAM 512 KB
	@ Region 5: FCRAM 128 MB
	@ Region 6: Exception vectors + ARM9 bootrom 64 KB
	@ Region 7: -
	ldr r0, =MAKE_REGION(0x01FF8000, REGION_32KB)
	mcr p15, 0, r0, c6, c0, 0
	ldr r0, =MAKE_REGION(0x08000000, REGION_2MB)
	mcr p15, 0, r0, c6, c1, 0
	ldr r0, =MAKE_REGION(0x10000000, REGION_2MB)
	mcr p15, 0, r0, c6, c2, 0
	ldr r0, =MAKE_REGION(0x18000000, REGION_4MB)
	mcr p15, 0, r0, c6, c3, 0
	ldr r0, =MAKE_REGION(0x1FF80000, REGION_512KB)
	mcr p15, 0, r0, c6, c4, 0
	ldr r0, =MAKE_REGION(0x20000000, REGION_128MB)
	mcr p15, 0, r0, c6, c5, 0
	ldr r0, =MAKE_REGION(0xFFFF0000, REGION_64KB)
	mcr p15, 0, r0, c6, c6, 0
	mov r0, #0
	mcr p15, 0, r0, c6, c7, 0

	@ Data access permissions:
	@ Region 0: User = RO, Privileged = RW
	@ Region 1: User = RW, Privileged = RW
	@ Region 2: User = RW, Privileged = RW
	@ Region 3: User = RW, Privileged = RW
	@ Region 4: User = RO, Privileged = RW
	@ Region 5: User = RW, Privileged = RW
	@ Region 6: User = RO, Privileged = RO
	@ Region 7: User = --, Privileged = --
	ldr r0, =MAKE_PERMISSIONS(PER_PRIV_RW_USR_RO, PER_PRIV_RW_USR_RW,
                              PER_PRIV_RW_USR_RW, PER_PRIV_RW_USR_RW,
                              PER_PRIV_RW_USR_RO, PER_PRIV_RW_USR_RW,
                              PER_PRIV_RO_USR_RO, PER_NO_ACC)
	mcr p15, 0, r0, c5, c0, 2   @ Data access permissions

	@ Instruction access permissions:
	@ Region 0: User = --, Privileged = --
	@ Region 1: User = RO, Privileged = RO
	@ Region 2: User = --, Privileged = --
	@ Region 3: User = --, Privileged = --
	@ Region 4: User = --, Privileged = --
	@ Region 5: User = RO, Privileged = RO
	@ Region 6: User = RO, Privileged = RO
	@ Region 7: User = --, Privileged = --
	ldr r0, =MAKE_PERMISSIONS(PER_NO_ACC, PER_PRIV_RO_USR_RO,
                              PER_NO_ACC, PER_NO_ACC,
                              PER_NO_ACC, PER_PRIV_RO_USR_RO,
                              PER_PRIV_RO_USR_RO, PER_NO_ACC)
	mcr p15, 0, r0, c5, c0, 3   @ Instruction access permissions

	@ Data cachable bits:
	@ Region 0 = yes
	@ Region 1 = yes
	@ Region 2 = no  <-- Never cache IO regs
	@ Region 3 = yes
	@ Region 4 = no
	@ Region 5 = yes
	@ Region 6 = yes
	@ Region 7 = no
	mov r0, #0b01101011
	mcr p15, 0, r0, c2, c0, 0   @ Data cachable bits

	@ Instruction cachable bits:
	@ Region 0 = no
	@ Region 1 = yes
	@ Region 2 = no  <-- Never cache IO regs
	@ Region 3 = no
	@ Region 4 = no
	@ Region 5 = yes
	@ Region 6 = yes
	@ Region 7 = no
	mov r0, #0b01100010
	mcr p15, 0, r0, c2, c0, 1   @ Instruction cachable bits

	@ Write bufferable bits:
	@ Region 0 = yes
	@ Region 1 = yes
	@ Region 2 = no  <-- Never buffer IO reg writes
	@ Region 3 = yes
	@ Region 4 = no
	@ Region 5 = yes
	@ Region 6 = no
	@ Region 7 = no
	mov r0, #0b00101011
	mcr p15, 0, r0, c3, c0, 0   @ Write bufferable bits

	mrc p15, 0, r0, c1, c0, 0   @ Read control register
	ldr r1, =0x1005             @ MPU, D-Cache and I-Cache bitmask
	orr r0, r0, r1              @ Enable MPU, D-Cache and I-Cache
	mcr p15, 0, r0, c1, c0, 0   @ Write control register
	bx lr


disableMpu:
	mov r3, lr
	bl flushDCache              @ Make sure all data has been written back
	mrc p15, 0, r0, c1, c0, 0   @ Read control register
	ldr r1, =0x1005             @ MPU, D-Cache and I-Cache bitmask
	bic r0, r0, r1              @ Disable MPU, D-Cache and I-Cache
	mcr p15, 0, r0, c1, c0, 0   @ Write control register
	bx r3
