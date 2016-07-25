/*
 *  AES code based on code from Normmatt
 *
 *  2016
 *  profi200
 */

#include <assert.h>
#include <string.h>
#include "types.h"
#include "mem_map.h"
#include "arm9/crypto.h"
#include "arm9/interrupt.h"
#include "arm9/ndma.h"
#include "cache.h"



//////////////////////////////////
//             AES              //
//////////////////////////////////

// TODO: Handle endianess!
static void addCounter(u32 *restrict ctr, u32 val)
{
	u32 carry, i = 1;
	u64 sum;


	sum = ctr[0];
	sum += (val>>4);
	carry = sum>>32;
	ctr[0] = sum & 0xFFFFFFFF;

	while(carry)
	{
		sum = ctr[i];
		sum += carry;
		carry = sum>>32;
		ctr[i] = sum & 0xFFFFFFFF;
		i++;
	}
}

// TODO: Handle endianess!
static void subCounter(u32 *restrict ctr, u32 val)
{
	u32 carry, i = 1;
	u32 sum;


	sum = ctr[0] - (val>>4);
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

void AES_setKey(u32 params, u8 keyslot, AesKeyType type, const u32 *restrict key, bool useTwlScrambler, bool updateKeyslot)
{
	REG_AESCNT = params;

	if(keyslot > 3) // CTR keyslot
	{
		REG_AESKEYCNT = keyslot | (useTwlScrambler<<6) | 0x80;
		for(u32 i = 0; i < 4; i++) REG_AESKEYFIFO[(u32)type] = key[i];
	}
	else // TWL keyslot
	{
		REG_AESKEYCNT = keyslot | 0x80;
		for(u32 i = 0; i < 4; i++) REG_AESKEY0[(u32)12 * keyslot + ((u32)type * 4) + i] = key[i];
	}

	REG_AESKEYSEL = keyslot;
	if(updateKeyslot) REG_AESCNT = AES_UPDATE_KEYSLOT;
}

void AES_selectKeyslot(u8 keyslot, bool updateKeyslot)
{
	REG_AESKEYSEL = keyslot;
	if(updateKeyslot) REG_AESCNT = AES_UPDATE_KEYSLOT;
}

void AES_setCtrIvNonce(AES_ctx *restrict ctx, const u32 *restrict ctrIvNonce, u32 params, u32 initialCtr)
{
	u32 ctrIvNonceSize, mode;
	if((mode = (params>>27 & 7)) > 1) ctrIvNonceSize = 4;
	else ctrIvNonceSize = 3;

	if(params & AES_INPUT_NORMAL)
	{
		for(u32 i = 0; i < ctrIvNonceSize; i++) ctx->ctrIvNonce[i] = ctrIvNonce[ctrIvNonceSize - 1 - i];
	}
	else for(u32 i = 0; i < ctrIvNonceSize; i++) ctx->ctrIvNonce[i] = ctrIvNonce[i];
	ctx->ctrIvNonceParams = params;

	// If cipher mode is CTR add the initial value to it. Can be 0.
	if(mode == 2) addCounter(ctx->ctrIvNonce, initialCtr);
}

u32* AES_getCtrIvNoncePtr(AES_ctx *restrict ctx)
{
	return ctx->ctrIvNonce;
}

void AES_setCryptParams(AES_ctx *restrict ctx, u32 params)
{
	ctx->aesParams = params;
}

static void setupNdma(const u32 *restrict in, u32 *restrict out, u32 wordCount, u32 burstSize)
{
	REG_NDMA0_CNT = 0;
	REG_NDMA0_SRC_ADDR = (u32)in;
	REG_NDMA0_DST_ADDR = (u32)REG_AESWRFIFO;
	REG_NDMA0_WRITE_CNT = wordCount;
	REG_NDMA0_BLOCK_CNT = NDMA_BLOCK_SYS_FREQ;
	REG_NDMA0_CNT = NDMA_ENABLE | NDMA_REPEATING_MODE | burstSize | NDMA_STARTUP_AES_IN |
					NDMA_SRC_UPDATE_INC | NDMA_DST_UPDATE_FIXED;

	REG_NDMA1_CNT = 0;
	REG_NDMA1_SRC_ADDR = (u32)REG_AESRDFIFO;
	REG_NDMA1_DST_ADDR = (u32)out;
	REG_NDMA1_WRITE_CNT = wordCount;
	REG_NDMA1_BLOCK_CNT = NDMA_BLOCK_SYS_FREQ;
	REG_NDMA1_CNT = NDMA_ENABLE | NDMA_REPEATING_MODE | burstSize | NDMA_STARTUP_AES_OUT |
					NDMA_SRC_UPDATE_FIXED | NDMA_DST_UPDATE_INC;
}

void AES_crypt(AES_ctx *restrict ctx, const u32 *restrict in, u32 *restrict out, u32 size)
{
	// DMA can't reach TCMs
	assert(((u32)in >= ITCM_BOOT9_MIRROR + ITCM_SIZE) && (((u32)in < DTCM_BASE) || ((u32)in >= DTCM_BASE + DTCM_SIZE)));
	assert(((u32)out >= ITCM_BOOT9_MIRROR + ITCM_SIZE) && (((u32)out < DTCM_BASE) || ((u32)out >= DTCM_BASE + DTCM_SIZE)));

	// Align to 16 bytes.
	size = (size + 0xFu) & ~0xFu;

	// Size is 4 words except for CCM mode.
	u32 mode, ctrIvNonceSize;
	if((mode = (ctx->aesParams>>27 & 7)) > 1) ctrIvNonceSize = 4;
	else ctrIvNonceSize = 3;

	// All writes must finish before using DMA
	flushDCacheRange(in, size);
	// Save the original out pointer for later invalidation
	const u32 *savedOut = out;

	u32 offset = 0;
	const u32 aesParams = AES_ENABLE | AES_IRQ_ENABLE | ctx->aesParams | AES_FLUSH_READ_FIFO | AES_FLUSH_WRITE_FIFO;
	while(offset < size)
	{
		u32 blockSize = ((size - offset > AES_MAX_BUF_SIZE) ? AES_MAX_BUF_SIZE : size - offset);

		// Check block alignment
		u32 aesDmaFifoSize, ndmaBurstSize;
		if(!(blockSize & 63))
		{
			aesDmaFifoSize = 3;
			ndmaBurstSize = NDMA_BURST_SIZE(16);
		}
		else if(!(blockSize & 31))
		{
			aesDmaFifoSize = 1;
			ndmaBurstSize = NDMA_BURST_SIZE(8);
		}
		else
		{
			aesDmaFifoSize = 0;
			ndmaBurstSize = NDMA_BURST_SIZE(4);
		}
		setupNdma(in, out, aesDmaFifoSize * 4 + 4, ndmaBurstSize);

		// Set CTR/IV/nonce
		REG_AESCNT = ctx->ctrIvNonceParams;
		for(u32 i = 0; i < ctrIvNonceSize; i++) REG_AESCTR[i] = ctx->ctrIvNonce[i];
		if(mode == 4) // AES_MODE_CBC_DECRYPT
		{
			// Save last 16 bytes of current input block as next IV for CBC decrypt
			if(ctx->ctrIvNonceParams & AES_INPUT_NORMAL)
			{
				for(u32 i = 0; i < 4; i++) ctx->ctrIvNonce[i] = in[(blockSize>>2) - 4 + 3 - i];
			}
			else for(u32 i = 0; i < 4; i++) ctx->ctrIvNonce[i] = in[(blockSize>>2) - 4 + i];
		}

		// Setup the AES engine and wait for it to finish
		REG_AESBLKCNT = (blockSize>>4)<<16;
		REG_AESCNT = aesParams | (aesDmaFifoSize<<14) | ((3 - aesDmaFifoSize)<<12);
		while(!(REG_IRQ_IF & (u32)IRQ_AES))
		{
			waitForIrq();
		}
		REG_IRQ_IF = (u32)IRQ_AES; // Aknowledge interrupt


		if(mode == 2) // AES_MODE_CTR
		{
			// Increase counter.
			addCounter(ctx->ctrIvNonce, blockSize);
		}
		else if(mode == 5) // AES_MODE_CBC_ENCRYPT
		{
			// Save last 16 bytes of current output block as next IV for CBC encrypt
			if(ctx->ctrIvNonceParams & AES_INPUT_NORMAL)
			{
				for(u32 i = 0; i < 4; i++) ctx->ctrIvNonce[i] = out[(blockSize>>2) - 4 + 3 - i];
			}
			else for(u32 i = 0; i < 4; i++) ctx->ctrIvNonce[i] = out[(blockSize>>2) - 4 + i];
		}

		in += blockSize>>2;
		out += blockSize>>2;
		offset += blockSize;
	}

	// Disable the NDMA channels
	REG_NDMA0_CNT = REG_NDMA1_CNT = 0;

	// Throw possibly cached lines out of the window
	invalidateDCacheRange(savedOut, size);
}

void AES_addCounter(AES_ctx *restrict ctx, u32 val)
{
	addCounter(ctx->ctrIvNonce, val);
}

void AES_subCounter(AES_ctx *restrict ctx, u32 val)
{
	subCounter(ctx->ctrIvNonce, val);
}



//////////////////////////////////
//             SHA              //
//////////////////////////////////

void SHA_start(u32 params)
{
	REG_SHA_CNT = SHA_ENABLE | params;
}

void SHA_update(const u32 *restrict data, u32 size)
{
	//const u32 *restrict dataPtr = data;

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

void SHA_finish(u32 *restrict hash, u32 endianess)
{
	REG_SHA_CNT = SHA_PAD_INPUT | endianess | (REG_SHA_CNT & (SHA_MODE_1 | SHA_MODE_224 | SHA_MODE_256));
	while(REG_SHA_CNT & SHA_ENABLE);

	u32 hashSize;
	switch(REG_SHA_CNT & (SHA_MODE_256 | SHA_MODE_224 | SHA_MODE_1))
	{
		case SHA_MODE_256:
			hashSize = 8;//32;
			break;
		case SHA_MODE_224:
			hashSize = 7;//28;
			break;
		case SHA_MODE_1:
			hashSize = 5;//20;
			break;
		default:
			return;
	}

	//memcpy(hash, REG_SHA_HASH, hashSize);
	for(u32 i = 0; i < hashSize; i++) hash[i] = REG_SHA_HASH[i];
}

void sha(const u32 *restrict data, u32 size, u32 *restrict hash, u32 params, u32 hashEndianess)
{
	SHA_start(params);
	SHA_update(data, size);
	SHA_finish(hash, hashEndianess);
}
