.arm
.cpu mpcore
.fpu vfpv2

.global invalidateICache
.global invalidateICacheRange
.global flushDCache
.global flushDCacheRange
.global invalidateDCache
.global invalidateDCacheRange

.type invalidateICache STT_FUNC
.type invalidateICacheRange STT_FUNC
.type flushDCache STT_FUNC
.type flushDCacheRange STT_FUNC
.type invalidateDCache STT_FUNC
.type invalidateDCacheRange STT_FUNC

.section ".text"


#define CACHE_LINE_SIZE	(32)



invalidateICache:
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0       @ Invalidate Entire Instruction Cache, also flushes the branch target cache
	@mcr p15, 0, r0, c7, c5, 6       @ Flush Entire Branch Target Cache
	mcr p15, 0, r0, c7, c10, 4      @ Data Synchronization Barrier
	mcr p15, 0, r0, c7, c5, 4       @ Flush Prefetch Buffer
	bx lr


invalidateICacheRange:
	add	r1, r1, r0
	bic	r0, r0, #(CACHE_LINE_SIZE - 1)
	mov r2, #0
invalidateICacheRange_lp:
	mcr	p15, 0, r0, c7, c5, 1       @ Invalidate Instruction Cache Line (using MVA)
	add	r0, r0, #CACHE_LINE_SIZE
	cmp	r0, r1
	blt	invalidateICacheRange_lp
	mcr p15, 0, r2, c7, c5, 6       @ Flush Entire Branch Target Cache
	mcr p15, 0, r2, c7, c10, 4      @ Data Synchronization Barrier
	mcr p15, 0, r2, c7, c5, 4       @ Flush Prefetch Buffer
	bx	lr


flushDCache:
	mov r0, #0
	mcr p15, 0, r0, c7, c10, 0      @ "Clean Entire Data Cache"
	mcr p15, 0, r0, c7, c10, 4      @ Data Synchronization Barrier
	bx lr


flushDCacheRange:
	add	r1, r1, r0
	bic	r0, r0, #(CACHE_LINE_SIZE - 1)
	mov r2, #0
flushDCacheRange_lp:
	mcr p15, 0, r0, c7, c10, 1      @ "Clean Data Cache Line (using MVA)"
	add	r0, r0, #CACHE_LINE_SIZE
	cmp	r0, r1
	blt	flushDCacheRange_lp
	mcr p15, 0, r2, c7, c10, 4      @ Data Synchronization Barrier
	bx lr


invalidateDCache:
	mov	r0, #0
	mcr p15, 0, r0, c7, c6, 0       @ Invalidate Entire Data Cache
	mcr p15, 0, r0, c7, c10, 4      @ Data Synchronization Barrier
	bx  lr


invalidateDCacheRange:
	add	r1, r1, r0
	tst	r0, #(CACHE_LINE_SIZE - 1)
	mcrne   p15, 0, r0, c7, c10, 1  @ "Clean Data Cache Line (using MVA)"
	tst	r1, #(CACHE_LINE_SIZE - 1)
	mcrne   p15, 0, r1, c7, c10, 1  @ "Clean Data Cache Line (using MVA)"
	bic	r0, r0, #(CACHE_LINE_SIZE - 1)
	mov r2, #0
invalidateDCacheRange_lp:
	mcr	p15, 0, r0, c7, c6, 1       @ Invalidate Data Cache Line (using MVA)
	add	r0, r0, #CACHE_LINE_SIZE
	cmp	r0, r1
	blt	invalidateDCacheRange_lp
	mcr p15, 0, r2, c7, c10, 4      @ Data Synchronization Barrier
	bx  lr
