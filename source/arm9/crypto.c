/*
 *  2017
 *  profi200
 */

#include <assert.h>
#include <string.h>
#include "mem_map.h"
#include "types.h"
#include "arm9/crypto.h"
#include "arm9/interrupt.h"
#include "arm9/ndma.h"
#include "cache.h"



//////////////////////////////////
//             AES              //
//////////////////////////////////

#define AES_REGS_BASE        (IO_MEM_ARM9_ONLY + 0x9000)
#define REG_AESCNT           *((vu32*)(AES_REGS_BASE + 0x000))

#define REG_AESBLKCNT        *((vu32*)(AES_REGS_BASE + 0x004))
#define REG_AES_BLKCNT_LOW   *((vu16*)(AES_REGS_BASE + 0x004))
#define REG_AES_BLKCNT_HIGH  *((vu16*)(AES_REGS_BASE + 0x006))
#define REG_AESWRFIFO         (        AES_REGS_BASE + 0x008)
#define REG_AESRDFIFO         (        AES_REGS_BASE + 0x00C)
#define REG_AESKEYSEL        *((vu8* )(AES_REGS_BASE + 0x010))
#define REG_AESKEYCNT        *((vu8* )(AES_REGS_BASE + 0x011))
#define REG_AESCTR            ((vu32*)(AES_REGS_BASE + 0x020))
#define REG_AESMAC            ((vu32*)(AES_REGS_BASE + 0x030))

#define REG_AESKEY0           ((vu32*)(AES_REGS_BASE + 0x040))
#define REG_AESKEYX0          ((vu32*)(AES_REGS_BASE + 0x050))
#define REG_AESKEYY0          ((vu32*)(AES_REGS_BASE + 0x060))
#define REG_AESKEY1           ((vu32*)(AES_REGS_BASE + 0x070))
#define REG_AESKEYX1          ((vu32*)(AES_REGS_BASE + 0x080))
#define REG_AESKEYY1          ((vu32*)(AES_REGS_BASE + 0x090))
#define REG_AESKEY2           ((vu32*)(AES_REGS_BASE + 0x0A0))
#define REG_AESKEYX2          ((vu32*)(AES_REGS_BASE + 0x0B0))
#define REG_AESKEYY2          ((vu32*)(AES_REGS_BASE + 0x0C0))
#define REG_AESKEY3           ((vu32*)(AES_REGS_BASE + 0x0D0))
#define REG_AESKEYX3          ((vu32*)(AES_REGS_BASE + 0x0E0))
#define REG_AESKEYY3          ((vu32*)(AES_REGS_BASE + 0x0F0))

#define REG_AESKEYFIFO       *((vu32*)(AES_REGS_BASE + 0x100))
#define REG_AESKEYXFIFO      *((vu32*)(AES_REGS_BASE + 0x104))
#define REG_AESKEYYFIFO      *((vu32*)(AES_REGS_BASE + 0x108))


void AES_init(void)
{
	REG_AESCNT = AES_MAC_SIZE(4) | AES_FLUSH_WRITE_FIFO | AES_FLUSH_READ_FIFO;
	*((vu8*)0x10000008) |= 0xCu; // ??

	REG_NDMA0_DST_ADDR = REG_AESWRFIFO;
	REG_NDMA0_INT_CNT = NDMA_INT_SYS_FREQ;
	REG_NDMA0_CNT = NDMA_REPEATING_MODE | NDMA_STARTUP_AES_IN |
	                NDMA_SRC_UPDATE_INC | NDMA_DST_UPDATE_FIXED;

	REG_NDMA1_SRC_ADDR = REG_AESRDFIFO;
	REG_NDMA1_INT_CNT = NDMA_INT_SYS_FREQ;
	REG_NDMA1_CNT = NDMA_REPEATING_MODE | NDMA_STARTUP_AES_OUT |
	                NDMA_SRC_UPDATE_FIXED | NDMA_DST_UPDATE_INC;

	REG_IRQ_IE |= 1u<<IRQ_AES;
}

void AES_setNormalKey(u8 keyslot, u8 orderEndianess, const u32 key[4])
{
	assert(keyslot < 0x40);
	assert(key != NULL);


	REG_AESCNT = (u32)orderEndianess<<23;
	if(keyslot > 3)
	{
		REG_AESKEYCNT = keyslot | 0x80u;
		REG_AESKEYFIFO = key[0];
		REG_AESKEYFIFO = key[1];
		REG_AESKEYFIFO = key[2];
		REG_AESKEYFIFO = key[3];
	}
	else
	{
		u32 lastu32;
		vu32 *twlKeyNReg = &REG_AESKEY0[12 * keyslot];
		if(orderEndianess & 4)
		{
			twlKeyNReg[0] = key[3];
			twlKeyNReg[1] = key[2];
			twlKeyNReg[2] = key[1];
			lastu32 = key[0];
		}
		else
		{
			twlKeyNReg[0] = key[0];
			twlKeyNReg[1] = key[1];
			twlKeyNReg[2] = key[2];
			lastu32 = key[3];
		}
		twlKeyNReg[3] = lastu32;
	}
}

void AES_setKeyX(u8 keyslot, u8 orderEndianess, bool useTwlScrambler, const u32 keyX[4])
{
	assert(keyslot < 0x40);
	assert(keyX != NULL);


	REG_AESCNT = (u32)orderEndianess<<23;
	if(keyslot > 3)
	{
		REG_AESKEYCNT = keyslot | (u8)useTwlScrambler<<6 | 0x80u;
		REG_AESKEYXFIFO = keyX[0];
		REG_AESKEYXFIFO = keyX[1];
		REG_AESKEYXFIFO = keyX[2];
		REG_AESKEYXFIFO = keyX[3];
	}
	else
	{
		u32 lastu32;
		vu32 *twlKeyNReg = &REG_AESKEY0[12 * keyslot];
		if(orderEndianess & 4)
		{
			twlKeyNReg[4] = keyX[3];
			twlKeyNReg[5] = keyX[2];
			twlKeyNReg[6] = keyX[1];
			lastu32 = keyX[0];
		}
		else
		{
			twlKeyNReg[4] = keyX[0];
			twlKeyNReg[5] = keyX[1];
			twlKeyNReg[6] = keyX[2];
			lastu32 = keyX[3];
		}
		twlKeyNReg[7] = lastu32;
	}
}

void AES_setKeyY(u8 keyslot, u8 orderEndianess, bool useTwlScrambler, const u32 keyY[4])
{
	assert(keyslot < 0x40);
	assert(keyY != NULL);


	REG_AESCNT = (u32)orderEndianess<<23;
	if(keyslot > 3)
	{
		REG_AESKEYCNT = keyslot | (u8)useTwlScrambler<<6 | 0x80u;
		REG_AESKEYYFIFO = keyY[0];
		REG_AESKEYYFIFO = keyY[1];
		REG_AESKEYYFIFO = keyY[2];
		REG_AESKEYYFIFO = keyY[3];
	}
	else
	{
		u32 lastu32;
		vu32 *twlKeyNReg = &REG_AESKEY0[12 * keyslot];
		if(orderEndianess & 4)
		{
			twlKeyNReg[8]  = keyY[3];
			twlKeyNReg[9]  = keyY[2];
			twlKeyNReg[10] = keyY[1];
			lastu32 = keyY[0];
		}
		else
		{
			twlKeyNReg[8]  = keyY[0];
			twlKeyNReg[9]  = keyY[1];
			twlKeyNReg[10] = keyY[2];
			lastu32 = keyY[3];
		}
		twlKeyNReg[11] = lastu32;
	}
}

void AES_selectKeyslot(u8 keyslot)
{
	assert(keyslot < 0x40);


	REG_AESKEYSEL = keyslot;
	REG_AESCNT |= AES_UPDATE_KEYSLOT;
}

void AES_setNonce(AES_ctx *const ctx, u8 orderEndianess, const u32 nonce[3])
{
	assert(ctx != NULL);
	assert(nonce != NULL);


	ctx->ctrIvNonceParams = (u32)orderEndianess<<23;
	u32 lastu32;
	if(orderEndianess & 4)
	{
		ctx->ctrIvNonce[0] = nonce[2];
		ctx->ctrIvNonce[1] = nonce[1];
		lastu32 = nonce[0];
	}
	else
	{
		ctx->ctrIvNonce[0] = nonce[0];
		ctx->ctrIvNonce[1] = nonce[1];
		lastu32 = nonce[2];
	}
	ctx->ctrIvNonce[2] = lastu32;
}

void AES_setCtrIv(AES_ctx *const ctx, u8 orderEndianess, const u32 ctrIv[4])
{
	assert(ctx != NULL);
	assert(ctrIv != NULL);


	ctx->ctrIvNonceParams = (u32)orderEndianess<<23;
	u32 lastu32;
	if(orderEndianess & 4)
	{
		ctx->ctrIvNonce[0] = ctrIv[3];
		ctx->ctrIvNonce[1] = ctrIv[2];
		ctx->ctrIvNonce[2] = ctrIv[1];
		lastu32 = ctrIv[0];
	}
	else
	{
		ctx->ctrIvNonce[0] = ctrIv[0];
		ctx->ctrIvNonce[1] = ctrIv[1];
		ctx->ctrIvNonce[2] = ctrIv[2];
		lastu32 = ctrIv[3];
	}
	ctx->ctrIvNonce[3] = lastu32;
}

// TODO: Handle endianess!
void AES_addCounter(u32 ctr[4], u32 val)
{
	u32 carry, i = 1;
	u64 sum;

	sum = ctr[0];
	sum += (val >> 4);
	carry = sum >> 32;
	ctr[0] = sum & 0xFFFFFFFFu;

	while(carry)
	{
		sum = ctr[i];
		sum += carry;
		carry = sum >> 32;
		ctr[i] = sum & 0xFFFFFFFFu;
		i++;
	}
}

void AES_subCounter(u32 ctr[4], u32 val)
{
	u32 carry, i = 1;
	u32 sum;

	sum = ctr[0] - (val >> 4);
	carry = (sum > ctr[0]);
	ctr[0] = sum;

	while(carry && i < 4)
	{
		sum = ctr[i] - carry;
		carry = (sum > ctr[i]);
		ctr[i] = sum;
		i++;
	}
}

void AES_setCryptParams(AES_ctx *const ctx, u8 inEndianessOrder, u8 outEndianessOrder)
{
	assert(ctx != NULL);

	ctx->aesParams = (u32)inEndianessOrder<<23 | (u32)outEndianessOrder<<22;
}

static void processBlocksCpu(const u32 *in, u32 *out, u32 blocks)
{
	REG_AES_BLKCNT_HIGH = blocks;
	REG_AESCNT |= AES_ENABLE | 3<<12 | AES_FLUSH_READ_FIFO | AES_FLUSH_WRITE_FIFO;

	for(u32 i = 0; i < blocks * 4; i += 4)
	{
		*((vu32*)REG_AESWRFIFO) = in[0 + i];
		*((vu32*)REG_AESWRFIFO) = in[1 + i];
		*((vu32*)REG_AESWRFIFO) = in[2 + i];
		*((vu32*)REG_AESWRFIFO) = in[3 + i];

		while(AES_READ_FIFO_COUNT == 0);

		out[0 + i] = *((vu32*)REG_AESRDFIFO);
		out[1 + i] = *((vu32*)REG_AESRDFIFO);
		out[2 + i] = *((vu32*)REG_AESRDFIFO);
		out[3 + i] = *((vu32*)REG_AESRDFIFO);
	}
}

// AES_init() must be called before this works
static void processBlocksDma(const u32 *in, u32 *out, u32 blocks)
{
	// DMA can't reach TCMs
	assert(((u32)in >= ITCM_BOOT9_MIRROR + ITCM_SIZE) && (((u32)in < DTCM_BASE) || ((u32)in >= DTCM_BASE + DTCM_SIZE)));
	assert(((u32)out >= ITCM_BOOT9_MIRROR + ITCM_SIZE) && (((u32)out < DTCM_BASE) || ((u32)out >= DTCM_BASE + DTCM_SIZE)));


	// Check block alignment
	u32 aesFifoSize, dmaBurstSize;
	if(!(blocks & 3))
	{
		aesFifoSize = 3;
		dmaBurstSize = NDMA_BURST_SIZE(16);
	}
	else if(!(blocks & 1))
	{
		aesFifoSize = 1;
		dmaBurstSize = NDMA_BURST_SIZE(8);
	}
	else
	{
		aesFifoSize = 0;
		dmaBurstSize = NDMA_BURST_SIZE(4);
	}

	REG_NDMA0_SRC_ADDR = (u32)in;
	REG_NDMA0_LOG_BLK_CNT = aesFifoSize * 4 + 4;
	REG_NDMA0_CNT = (REG_NDMA0_CNT & 0xFFF0FFFFu) | NDMA_ENABLE | dmaBurstSize;

	REG_NDMA1_DST_ADDR = (u32)out;
	REG_NDMA1_LOG_BLK_CNT = aesFifoSize * 4 + 4;
	REG_NDMA1_CNT = (REG_NDMA1_CNT & 0xFFF0FFFFu) | NDMA_ENABLE | dmaBurstSize;

	REG_AES_BLKCNT_HIGH = blocks;
	REG_AESCNT |= AES_ENABLE | AES_IRQ_ENABLE | aesFifoSize<<14 | (3 - aesFifoSize)<<12 |
	              AES_FLUSH_READ_FIFO | AES_FLUSH_WRITE_FIFO;
	while(REG_AESCNT & AES_ENABLE)
	{
		waitForIrq();
	}

	// Disable the NDMA channels
	REG_NDMA0_CNT = (REG_NDMA0_CNT<<1)>>1;
	REG_NDMA1_CNT = (REG_NDMA1_CNT<<1)>>1;
}

void AES_ctr(AES_ctx *const ctx, const u32 *in, u32 *out, u32 blocks, bool dma)
{
	assert(ctx != NULL);
	assert(in != NULL);
	assert(out != NULL);

	const u32 ctrParams = ctx->ctrIvNonceParams;
	u32 *const ctr = ctx->ctrIvNonce;
	const u32 aesParams = AES_MODE_CTR | ctx->aesParams;


	if(dma)
	{
		flushDCacheRange(in, blocks<<4);
		invalidateDCacheRange(out, blocks<<4);
	}

	while(blocks)
	{
		REG_AESCNT = ctrParams;
		REG_AESCTR[0] = ctr[0];
		REG_AESCTR[1] = ctr[1];
		REG_AESCTR[2] = ctr[2];
		REG_AESCTR[3] = ctr[3];

		REG_AESCNT = aesParams;
		u32 blockNum = ((blocks > AES_MAX_BLOCKS) ? AES_MAX_BLOCKS : blocks);
		if(dma) processBlocksDma(in, out, blockNum);
		else processBlocksCpu(in, out, blockNum);

		AES_addCounter(ctr, blockNum<<4);
		in += blockNum<<2;
		out += blockNum<<2;
		blocks -= blockNum;
	}
}



//////////////////////////////////
//             SHA              //
//////////////////////////////////

#define SHA_REGS_BASE   (IO_MEM_ARM9_ONLY + 0xA000)
#define REG_SHA_CNT     *((vu32*)(SHA_REGS_BASE + 0x00))
#define REG_SHA_BLKCNT  *((vu32*)(SHA_REGS_BASE + 0x04))
#define REG_SHA_HASH     ((u32* )(SHA_REGS_BASE + 0x40))
#define REG_SHA_INFIFO   (       (SHA_REGS_BASE + 0x80))


void SHA_start(u8 params)
{
	REG_SHA_CNT = SHA_ENABLE | (u32)params;
}

void SHA_update(const u32 *data, u32 size)
{
	while(size >= 0x40)
	{
		for(u32 i = 0; i < 4; i++)
		{
			((vu32*)REG_SHA_INFIFO)[0 + i] = *data++;
			((vu32*)REG_SHA_INFIFO)[1 + i] = *data++;
			((vu32*)REG_SHA_INFIFO)[2 + i] = *data++;
			((vu32*)REG_SHA_INFIFO)[3 + i] = *data++;
		}
		while(REG_SHA_CNT & SHA_ENABLE);

		size -= 0x40;
	}

	if(size) memcpy((void*)REG_SHA_INFIFO, data, size);
}

void SHA_finish(u32 *const hash, u8 endianess)
{
	REG_SHA_CNT = (REG_SHA_CNT & (SHA_MODE_1 | SHA_MODE_224 | SHA_MODE_256)) | (u32)endianess | SHA_PAD_INPUT;
	while(REG_SHA_CNT & SHA_ENABLE);

	u32 hashSize;
	switch(REG_SHA_CNT & (SHA_MODE_1 | SHA_MODE_224 | SHA_MODE_256))
	{
		case SHA_MODE_256:
			hashSize = 8; // 32;
			break;
		case SHA_MODE_224:
			hashSize = 7; // 28;
			break;
		case SHA_MODE_1:
			hashSize = 5; // 20;
			break;
		default:
			return;
	}

	for(u32 i = 0; i < hashSize; i++) hash[i] = REG_SHA_HASH[i];
}

void sha(const u32 *data, u32 size, u32 *const hash, u8 params, u8 hashEndianess)
{
	SHA_start(params);
	SHA_update(data, size);
	SHA_finish(hash, hashEndianess);
}
