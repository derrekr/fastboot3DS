/*
 *  AES code based on code from Normmatt
 */


#include <stdbool.h>
#include <string.h>
#include "types.h"
#include "crypto.h"



//////////////////////////////////
//             AES              //
//////////////////////////////////

// TODO: Handle endianess!
static void addCounter(void *restrict ctr, u32 val)
{
	u32 *restrict ctr32 = ctr;
	u32 carry, i = 1;
	u64 sum;


	sum = ctr32[0];
	sum += (val>>4);
	carry = sum>>32;
	ctr32[0] = sum & 0xFFFFFFFF;

	while(carry)
	{
		sum = ctr32[i];
		sum += carry;
		carry = sum>>32;
		ctr32[i] = sum & 0xFFFFFFFF;
		i++;
	}
}

// TODO: Handle endianess!
static void subCounter(void *restrict ctr, u32 val)
{
	u32 *restrict ctr32 = ctr;
	u32 carry, i = 1;
	u32 sum;


	sum = ctr32[0] - (val>>4);
	carry = (sum > ctr32[0]);
	ctr32[0] = sum;

	while(carry && i < 4)
	{
		sum = ctr32[i] - carry;
		carry = (sum > ctr32[i]);
		ctr32[i] = sum;
		i++;
	}
}

void AES_setTwlNormalKey(u32 params, u8 keyslot, const void *restrict twlNormalKey)
{
	const u32 *restrict key = twlNormalKey;

	REG_AESCNT = params;
	REG_AESKEYCNT = keyslot | 0xC0;
	for(int i = 0; i < 4; i++) REG_AESKEY0[i + keyslot * 12] = key[i];
	REG_AESKEYSEL = keyslot;
	REG_AESCNT = AES_UPDATE_KEYSLOT;
}
void AES_setTwlKeyY(u32 params, u8 keyslot, const void *restrict twlKeyY)
{
	const u32 *restrict key = twlKeyY;

	REG_AESCNT = params;
	REG_AESKEYCNT = keyslot | 0xC0;
	for(int i = 0; i < 4; i++) REG_AESKEYY0[i + keyslot * 12] = key[i];
	REG_AESKEYSEL = keyslot;
	REG_AESCNT = AES_UPDATE_KEYSLOT;
}
void AES_setTwlKeyX(u32 params, u8 keyslot, const void *restrict twlKeyX)
{
	const u32 *restrict key = twlKeyX;

	REG_AESCNT = params;
	REG_AESKEYCNT = keyslot | 0xC0;
	for(int i = 0; i < 4; i++) REG_AESKEYX0[i + keyslot * 12] = key[i];
	REG_AESKEYSEL = keyslot;
	REG_AESCNT = AES_UPDATE_KEYSLOT;
}

void AES_setNormalKey(u32 params, u8 keyslot, const void *restrict normalKey)
{
	const u32 *restrict key = normalKey;

	REG_AESCNT = params;
	REG_AESKEYCNT = keyslot | 0x80;
	for(int i = 0; i < 4; i++) REG_AESKEYFIFO = key[i];
	REG_AESKEYSEL = keyslot;
	REG_AESCNT = AES_UPDATE_KEYSLOT;
}

void AES_setKeyY(u32 params, u8 keyslot, const void *restrict keyY, bool useTwlScrambler)
{
	const u32 *restrict key = keyY;

	REG_AESCNT = params;
	REG_AESKEYCNT = keyslot | (useTwlScrambler<<6) | 0x80;
	for(int i = 0; i < 4; i++) REG_AESKEYYFIFO = key[i];
	REG_AESKEYSEL = keyslot;
	REG_AESCNT = AES_UPDATE_KEYSLOT;
}

void AES_setKeyX(u32 params, u8 keyslot, const void *restrict keyX, bool useTwlScrambler)
{
	const u32 *restrict key = keyX;

	REG_AESCNT = params;
	REG_AESKEYCNT = keyslot | (useTwlScrambler<<6) | 0x80;
	for(int i = 0; i < 4; i++) REG_AESKEYXFIFO = key[i];
	REG_AESKEYSEL = keyslot;
	REG_AESCNT = AES_UPDATE_KEYSLOT;
}

void AES_selectKeyslot(u8 keyslot)
{
	REG_AESKEYSEL = keyslot;
	REG_AESCNT = AES_UPDATE_KEYSLOT;
}

void AES_setCtrIvNonce(AES_ctx *restrict ctx, const void *restrict ctrIvNonce, u32 params, u32 initialCtr)
{
	const u32 *restrict inCtrIvNonce = ctrIvNonce;
	int ctrIvNonceSize;


	if(((params>>27) & 7) > 1) ctrIvNonceSize = 4;
	else ctrIvNonceSize = 3;

	if(params & AES_INPUT_NORMAL_ORDER)
	{
		for(int i = 0; i < ctrIvNonceSize; i++) ctx->ctrIvNonce[i] = inCtrIvNonce[ctrIvNonceSize - 1 - i];
	}
	else for(int i = 0; i < ctrIvNonceSize; i++) ctx->ctrIvNonce[i] = inCtrIvNonce[i];

	ctx->ctrIvNonceEndianess = params & AES_INPUT_BIG; // Mask for input endianess.

	// If cipher mode is CTR add the initial value to it. Can be 0.
	if(((params>>27) & 7) == 2) addCounter(ctx->ctrIvNonce, initialCtr);

	// Set CTR/IV/nonce.
	REG_AESCNT = ctx->ctrIvNonceEndianess;
	for(int i = 0; i < ctrIvNonceSize; i++) REG_AESCTR[i] = ctx->ctrIvNonce[i];
}

void* AES_getCtrIvNoncePtr(AES_ctx *restrict ctx)
{
	return ctx->ctrIvNonce;
}

void AES_setCryptParams(AES_ctx *restrict ctx, u32 params)
{
	ctx->aesParams = params;
}

void AES_crypt(AES_ctx *restrict ctx, const void *restrict in, void *restrict out, u32 size)
{
	// Align to 16 bytes.
	size = (size + 0xf) & ~0xf;

	const u32 *restrict inBuf = in;
	u32 *restrict outBuf = out;
	u32 blockSize;
	u32 offset = 0;
	u32 mode;
	int ctrIvNonceSize;


	// Size is 4 words except for CCM mode.
	if(((mode = ((ctx->aesParams>>27) & 7))) > 1) ctrIvNonceSize = 4;
	else ctrIvNonceSize = 3;

	while(offset < size)
	{
		blockSize = ((size - offset > AES_MAX_BUF_SIZE) ? AES_MAX_BUF_SIZE : size - offset);

		REG_AESBLKCNT = (blockSize>>4)<<16;
		REG_AESCNT = AES_ENABLE | ctx->aesParams;

		for(u32 j = 0; j < blockSize>>2; j += 4)
		{
			REG_AESWRFIFO = inBuf[0 + j];
			REG_AESWRFIFO = inBuf[1 + j];
			REG_AESWRFIFO = inBuf[2 + j];
			REG_AESWRFIFO = inBuf[3 + j];

			while(AES_READ_FIFO_COUNT != 4);

			outBuf[0 + j] = REG_AESRDFIFO;
			outBuf[1 + j] = REG_AESRDFIFO;
			outBuf[2 + j] = REG_AESRDFIFO;
			outBuf[3 + j] = REG_AESRDFIFO;
		}

		inBuf += blockSize>>2;
		outBuf += blockSize>>2;
		offset += blockSize;

		if(mode == 2) // AES_MODE_CTR
		{
			// Increase counter.
			addCounter(ctx->ctrIvNonce, blockSize);

			REG_AESCNT = ctx->ctrIvNonceEndianess; // CTR/IV/NONCE endianess
			for(int i = 0; i < ctrIvNonceSize; i++) REG_AESCTR[i] = ctx->ctrIvNonce[i];
		}
	}
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
	REG_SHA_CNT = SHA_NORMAL_ROUND | params;
}

void SHA_update(const void *restrict data, u32 size)
{
	const u32 *restrict dataPtr = data;

	while(size >= 0x40)
	{
		for(int i = 0; i < 4; i++)
		{
			REG_SHA_INFIFO[0 + i] = *dataPtr++;
			REG_SHA_INFIFO[1 + i] = *dataPtr++;
			REG_SHA_INFIFO[2 + i] = *dataPtr++;
			REG_SHA_INFIFO[3 + i] = *dataPtr++;
		}
		while(REG_SHA_CNT & SHA_NORMAL_ROUND);

		size -= 0x40;
	}

	if(size)
	{
		memcpy((u32*)REG_SHA_INFIFO, dataPtr, size);
		while(REG_SHA_CNT & SHA_NORMAL_ROUND);
	}
}

void SHA_finish(void *restrict hash, u32 endianess)
{
	REG_SHA_CNT = SHA_FINAL_ROUND | endianess | (REG_SHA_CNT & (SHA_MODE_256 | SHA_MODE_224 | SHA_MODE_1));
	while(REG_SHA_CNT & SHA_FINAL_ROUND);

	u32 hashSize;
	switch(REG_SHA_CNT & (SHA_MODE_256 | SHA_MODE_224 | SHA_MODE_1))
	{
		case SHA_MODE_256:
			hashSize = 32;
			break;
		case SHA_MODE_224:
			hashSize = 28;
			break;
		case SHA_MODE_1:
			hashSize = 20;
			break;
		default:
			return;
	}

	memcpy(hash, REG_SHA_HASH, hashSize);
}

void sha(const void *restrict data, u32 size, void *restrict hash, u32 params, u32 hashEndianess)
{
	SHA_start(params);
	SHA_update(data, size);
	SHA_finish(hash, hashEndianess);
}
