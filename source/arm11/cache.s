.arm
.cpu mpcore
.fpu vfpv2

.global invalidateICache
.global flushDCache

.type invalidateICache STT_FUNC
.type flushDCache STT_FUNC

.section ".text"



invalidateICache:
	@ Invalidate Entire Instruction Cache,
	@ also flushes the branch target cache
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0
	mcr p15, 0, r0, c7, c5, 4  @ Flush Prefetch Buffer
	mcr p15, 0, r0, c7, c10, 4 @ Data Synchronization Barrier
	bx lr


flushDCache:
	@ Clear and Invalidate Entire Data Cache
	mov r0, #0
	mcr p15, 0, r0, c7, c14, 0
	mcr p15, 0, r0, c7, c10, 4 @ Data Synchronization Barrier
	bx lr
