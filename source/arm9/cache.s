.arm
.arch armv5te
.fpu softvfp

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

#define ICACHE_SIZE	0x2000
#define DCACHE_SIZE	0x1000
#define CACHE_LINE_SIZE	32



invalidateICache:
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0
	bx	lr


invalidateICacheRange:
	add	r1, r1, r0
	bic	r0, r0, #CACHE_LINE_SIZE - 1
invalidateI:
	mcr	p15, 0, r0, c7, c5, 1
	add	r0, r0, #CACHE_LINE_SIZE
	cmp	r0, r1
	blt	invalidateI
	bx	lr


flushDCache:
	mov	r1, #0
outer_loop:
	mov	r0, #0
inner_loop:
	orr	r2, r1, r0              @ generate segment and line address
	mcr	p15, 0, r2, c7, c14, 2  @ clean and flush the line
	add	r0, r0, #CACHE_LINE_SIZE
	cmp	r0, #DCACHE_SIZE/4
	bne	inner_loop
	add	r1, r1, #0x40000000
	cmp	r1, #0
	bne	outer_loop
	b   drainWriteBuffer


flushDCacheRange:
	add	r1, r1, r0
	bic	r0, r0, #(CACHE_LINE_SIZE - 1)
flush:
	mcr	p15, 0, r0, c7, c14, 1  @ clean and flush address
	add	r0, r0, #CACHE_LINE_SIZE
	cmp	r0, r1
	blt	flush

drainWriteBuffer:
	mov r0, #0
	mcr p15, 0, r0, c7, c10, 4  @ drain write buffer
	bx  lr


invalidateDCache:
	mov	r0, #0
	mcr	p15, 0, r0, c7, c6, 0
	bx  lr


invalidateDCacheRange:
	add	r1, r1, r0
	tst	r0, #CACHE_LINE_SIZE - 1
	mcrne   p15, 0, r0, c7, c10, 1  @ clean D entry
	tst	r1, #CACHE_LINE_SIZE - 1
	mcrne   p15, 0, r1, c7, c10, 1  @ clean D entry
	bic	r0, r0, #CACHE_LINE_SIZE - 1
invalidateD:
	mcr	p15, 0, r0, c7, c6, 1
	add	r0, r0, #CACHE_LINE_SIZE
	cmp	r0, r1
	blt	invalidateD
	bx  lr
