.arm
.cpu arm946e-s
.fpu softvfp

.global invalidateICache
.global invalidateICacheRange
.global flushDCache
.global flushInvalidateDCache
.global flushDCacheRange
.global flushInvalidateDCacheRange
.global invalidateDCache
.global invalidateDCacheRange

.type invalidateICache STT_FUNC
.type invalidateICacheRange STT_FUNC
.type flushDCache STT_FUNC
.type flushInvalidateDCache STT_FUNC
.type flushDCacheRange STT_FUNC
.type flushInvalidateDCacheRange STT_FUNC
.type invalidateDCache STT_FUNC
.type invalidateDCacheRange STT_FUNC

.section ".text"


#define ICACHE_SIZE     (0x2000)
#define DCACHE_SIZE     (0x1000)
#define CACHE_LINE_SIZE	(32)



invalidateICache:
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0       @ "Flush instruction cache"
	bx lr


invalidateICacheRange:
	add r1, r1, r0
	bic r0, r0, #(CACHE_LINE_SIZE - 1)
	invalidateICacheRange_lp:
		mcr p15, 0, r0, c7, c5, 1   @ "Flush instruction cache single entry Address"
		add r0, r0, #CACHE_LINE_SIZE
		cmp r0, r1
		blt invalidateICacheRange_lp
	bx lr


flushDCache:
	mov r1, #0
	flushDCache_outer_lp:
		mov r0, #0
		flushDCache_inner_lp:
			orr r2, r1, r0             @ Generate segment and line address
			mcr p15, 0, r2, c7, c10, 2 @ "Clean data cache entry Index and segment"
			add r0, r0, #CACHE_LINE_SIZE
			cmp r0, #(DCACHE_SIZE / 4)
			bne flushDCache_inner_lp
		add r1, r1, #0x40000000
		cmp r1, #0
		bne flushDCache_outer_lp
	b drainWriteBufferFlushInvalidate


flushInvalidateDCache:
	mov r1, #0
	flushInvalidateDCache_outer_lp:
		mov r0, #0
		flushInvalidateDCache_inner_lp:
			orr r2, r1, r0             @ Generate segment and line address
			mcr p15, 0, r2, c7, c14, 2 @ "Clean and flush data cache entry Index and segment"
			add r0, r0, #CACHE_LINE_SIZE
			cmp r0, #(DCACHE_SIZE / 4)
			bne flushInvalidateDCache_inner_lp
		add r1, r1, #0x40000000
		cmp r1, #0
		bne flushInvalidateDCache_outer_lp
drainWriteBufferFlushInvalidate:
	mcr p15, 0, r1, c7, c10, 4         @ Drain write buffer
	bx lr


flushDCacheRange:
	add r1, r1, r0
	bic r0, r0, #(CACHE_LINE_SIZE - 1)
	flushDCacheRange_lp:
		mcr p15, 0, r0, c7, c10, 1  @ "Clean data cache entry Address"
		add r0, r0, #CACHE_LINE_SIZE
		cmp r0, r1
		blt flushDCacheRange_lp
	b drainWriteBufferFlushInvalidateRange


flushInvalidateDCacheRange:
	add r1, r1, r0
	bic r0, r0, #(CACHE_LINE_SIZE - 1)
	flushInvalidateDCacheRange_lp:
		mcr p15, 0, r0, c7, c14, 1  @ "Clean and flush data cache entry Address"
		add r0, r0, #CACHE_LINE_SIZE
		cmp r0, r1
		blt flushInvalidateDCacheRange_lp
drainWriteBufferFlushInvalidateRange:
	mov r0, #0
	mcr p15, 0, r0, c7, c10, 4      @ Drain write buffer
	bx lr


invalidateDCache:
	mov r0, #0
	mcr p15, 0, r0, c7, c6, 0       @ "Flush data cache"
	bx lr


invalidateDCacheRange:
	add r1, r1, r0
	tst r0, #(CACHE_LINE_SIZE - 1)
	mcrne p15, 0, r0, c7, c10, 1    @ "Clean data cache entry Address"
	tst r1, #(CACHE_LINE_SIZE - 1)
	mcrne p15, 0, r1, c7, c10, 1    @ "Clean data cache entry Address"
	bic r0, r0, #(CACHE_LINE_SIZE - 1)
	invalidateDCacheRange_lp:
		mcr p15, 0, r0, c7, c6, 1   @ "Flush data cache single entry Address"
		add r0, r0, #CACHE_LINE_SIZE
		cmp r0, r1
		blt invalidateDCacheRange_lp
	bx lr
