#pragma once

#include "types.h"


//////////////////////////////////
//             AES              //
//////////////////////////////////

#define AES_MAX_BUF_SIZE           (0xFFFF0)

#define REG_AESCNT                 (*(vu32*)0x10009000)
#define REG_AESBLKCNT              (*(vu32*)0x10009004)
#define REG_AESBLKCNTH1            (*(vu16*)0x10009004)
#define REG_AESBLKCNTH2            (*(vu16*)0x10009006)
#define REG_AESWRFIFO              (*(vu32*)0x10009008)
#define REG_AESRDFIFO              (*(vu32*)0x1000900C)
#define REG_AESKEYSEL              (*(vu8*) 0x10009010)
#define REG_AESKEYCNT              (*(vu8*) 0x10009011)
#define REG_AESCTR                 ( (vu32*)0x10009020) //16
#define REG_AESMAC                 ( (vu32*)0x10009030) //16

#define REG_AESKEY0                ( (vu32*)0x10009040)
#define REG_AESKEYX0               ( (vu32*)0x10009050)
#define REG_AESKEYY0               ( (vu32*)0x10009060)
#define REG_AESKEY1                ( (vu32*)0x10009070)
#define REG_AESKEYX1               ( (vu32*)0x10009080)
#define REG_AESKEYY1               ( (vu32*)0x10009090)
#define REG_AESKEY2                ( (vu32*)0x100090A0)
#define REG_AESKEYX2               ( (vu32*)0x100090B0)
#define REG_AESKEYY2               ( (vu32*)0x100090C0)
#define REG_AESKEY3                ( (vu32*)0x100090D0)
#define REG_AESKEYX3               ( (vu32*)0x100090E0)
#define REG_AESKEYY3               ( (vu32*)0x100090F0)

#define REG_AESKEYFIFO             (*(vu32*)0x10009100)
#define REG_AESKEYXFIFO            (*(vu32*)0x10009104)
#define REG_AESKEYYFIFO            (*(vu32*)0x10009108)

#define AES_WRITE_FIFO_COUNT       ((REG_AESCNT>>0) & 0x1F)
#define AES_READ_FIFO_COUNT        ((REG_AESCNT>>5) & 0x1F)
#define AES_BUSY                   (1<<31)

#define AES_FLUSH_READ_FIFO        (1<<10)
#define AES_FLUSH_WRITE_FIFO       (1<<11)
#define AES_BIT12                  (1<<12)
#define AES_BIT13                  (1<<13)
#define AES_MAC_SIZE(n)            ((n & 7)<<16)
#define AES_MAC_REGISTER_SOURCE    (1<<20)
#define AES_MAC_STATUS             (1<<21) // AES_UNKNOWN_21
#define AES_OUTPUT_BIG             (1<<22)
#define AES_OUTPUT_LITTLE          (0)
#define AES_INPUT_BIG              (1<<23)
#define AES_INPUT_LITTLE           (0)
#define AES_OUTPUT_NORMAL_ORDER    (1<<24)
#define AES_OUTPUT_REVERSED_ORDER  (0)
#define AES_INPUT_NORMAL_ORDER     (1<<25)
#define AES_INPUT_REVERSED_ORDER   (0)
#define AES_UPDATE_KEYSLOT         (1<<26) // AES_UNKNOWN_26
#define AES_INTERRUPT_ENABLE       (1<<30)
#define AES_ENABLE                 (1<<31)

#define AES_MODE_CCM_DECRYPT       (0)
#define AES_MODE_CCM_ENCRYPT       (1<<27)
#define AES_MODE_CTR               (2<<27)
#define AES_MODE_CBC_DECRYPT       (4<<27)
#define AES_MODE_CBC_ENCRYPT       (5<<27)
#define AES_MODE_ECB_DECRYPT       (6<<27)
#define AES_MODE_ECB_ENCRYPT       (7<<27)


typedef struct
{
	u32 ctrIvNonce[4];
	u32 ctrIvNonceEndianess;
	u32 aesParams;
} AES_ctx;


/**
 * @brief      Selects keyslot and sets TWL (DSi) normal key for regular AES
 *             en-/decryption.
 *
 * @param[in]  params   Word order and endianess bitmask.
 * @param[in]  keyslot  The keyslot this key pair will be set for. Must be <4.
 * @param[in]  key      Pointer to 128-bit AES normal key data.
 */
void AES_setTwlNormalKey(u32 params, u8 keyslot, const u32 *restrict key);

/**
 * @brief      Selects keyslot and sets TWL (DSi) keyY.
 *
 * @param[in]  params   Word order and endianess bitmask.
 * @param[in]  keyslot  The keyslot this key pair will be set for. Must be <4.
 * @param[in]  keyY     Pointer to 128-bit AES keyY data.
 */
void AES_setTwlKeyY(u32 params, u8 keyslot, const u32 *restrict keyY);

/**
 * @brief      Selects keyslot and sets TWL (DSi) keyX.
 *
 * @param[in]  params   Word order and endianess bitmask.
 * @param[in]  keyslot  The keyslot this key pair will be set for. Must be <4.
 * @param[in]  keyX     Pointer to 128-bit AES keyX data.
 */
void AES_setTwlKeyX(u32 params, u8 keyslot, const u32 *restrict keyX);

/**
 * @brief      Selects keyslot and sets normal key for regular AES
 *             en-/decryption.
 *
 * @param[in]  params   Word order and endianess bitmask.
 * @param[in]  keyslot  The keyslot this normal key will be set for.
 * @param[in]  key      Pointer to 128-bit AES normal key data.
 */
void AES_setNormalKey(u32 params, u8 keyslot, const u32 *restrict key);

/**
 * @brief      Selects keyslot and sets keyY. This updates the internal normal key.
 *
 * @param[in]  params           Word order and endianess bitmask.
 * @param[in]  keyslot          The keyslot this keyY will be set for. Must be >3.
 * @param[in]  keyY             Pointer to 128-bit AES keyY data.
 * @param[in]  useTwlScrambler  bool true if TWL keyscrambler is used instead of CTR keyscrambler.
 */
void AES_setKeyY(u32 params, u8 keyslot, const u32 *restrict keyY, bool useTwlScrambler);

/**
 * @brief      Selects keyslot and sets keyX. This does not updates the internal normal key.
 * @brief      A keyY must be set after this to update the internal normal key.
 *
 * @param[in]  params   Word order and endianess bitmask.
 * @param[in]  keyslot  The keyslot this keyX will be set for. Must be >3.
 * @param[in]  keyX     Pointer to 128-bit AES keyX data.
 * @param[in]  useTwlScrambler  bool true if TWL keyscrambler is used instead of CTR keyscrambler.
 */
void AES_setKeyX(u32 params, u8 keyslot, const u32 *restrict keyX, bool useTwlScrambler);

/**
 * @brief      Selects the given keyslot for all following crypto operations.
 *
 * @param[in]  keyslot  The keyslot to select.
 */
void AES_selectKeyslot(u8 keyslot);

/**
 * @brief      Copies the given CTR/IV/nonce into internal state.
 *
 * @param      ctx         Pointer to AES_ctx (AES context).
 * @param[in]  ctrIvNonce  Pointer to CTR/IV/nonce data. Size is determined by params.
 * @param[in]  params      Word order, endianess and AES cipher mode bitmask.
 * @param[in]  initialCtr  Value to update the counter in CTR mode with. Can be 0.
 */
void AES_setCtrIvNonce(AES_ctx *restrict ctx, const u32 *restrict ctrIvNonce, u32 params, u32 initialCtr);

/**
 * @brief      Returns a pointer to the CTR/IV/nonce stored in internal state.
 *
 * @param      ctx   Pointer to AES_ctx (AES context).
 *
 * @return     A pointer to the internal CTR/IV/nonce data.
 */
u32* AES_getCtrIvNoncePtr(AES_ctx *restrict ctx);

/**
 * @brief      Sets params in internal state for all following crypto operations.
 *
 * @param      ctx     Pointer to AES_ctx (AES context).
 * @param[in]  params  Params bitmask to set.
 */
void AES_setCryptParams(AES_ctx *restrict ctx, u32 params);

/**
 * @brief      En-/decrypts data with the previosly set params.
 * @brief      In CTR mode the internal counter is updated after each call.
 *
 * @param      ctx   Pointer to AES_ctx (AES context).
 * @param[in]  in    In data pointer. Can be the same as out.
 * @param      out   Out data pointer. Can be the same as in.
 * @param[in]  size  Data size. If not 16 bytes aligned it is rounded up.
 */
void AES_crypt(AES_ctx *restrict ctx, const void *restrict in, void *restrict out, u32 size);

/**
 * @brief      Increments the internal counter with the given value.
 *
 * @param      ctx   Pointer to AES_ctx (AES context).
 * @param[in]  val   Value to add to the counter.
 */
void AES_addCounter(AES_ctx *restrict ctx, u32 val); // TODO: Handle endianess!

/**
 * @brief      Decrements the internal counter with the given value.
 *
 * @param      ctx   Pointer to AES_ctx (AES context).
 * @param[in]  val   Value to substract from the counter.
 */
void AES_subCounter(AES_ctx *restrict ctx, u32 val);



//////////////////////////////////
//             SHA              //
//////////////////////////////////

#define REG_SHA_CNT        (*((vu32*)0x1000A000))
#define REG_SHA_BLKCNT     (*((vu32*)0x1000A004))
#define REG_SHA_HASH       ( ((u32*) 0x1000A040))
#define REG_SHA_INFIFO     ( ((vu32*)0x1000A080))

#define SHA_ENABLE         (1) // Also used as busy flag
#define SHA_PAD_INPUT      (1<<1)
#define SHA_INPUT_BIG      (1<<3)
#define SHA_INPUT_LITTLE   (0)
#define SHA_OUTPUT_BIG     SHA_INPUT_BIG
#define SHA_OUTPUT_LITTLE  SHA_INPUT_LITTLE

#define SHA_MODE_256       (0)
#define SHA_MODE_224       (1<<4)
#define SHA_MODE_1         (1<<5)


/**
 * @brief      Sets input mode, endianess and starts the hash operation.
 *
 * @param[in]  params  Mode and input endianess bitmask.
 */
void SHA_start(u32 params);

/**
 * @brief      Hashes the data pointed to.
 *
 * @param[in]  data  Pointer to data to hash.
 * @param[in]  size  Size of the data to hash.
 */
void SHA_update(const void *restrict data, u32 size);

/**
 * @brief      Generates the final hash.
 *
 * @param      hash       Pointer to memory to copy the hash to.
 * @param[in]  endianess  Endianess bitmask for the hash.
 */
void SHA_finish(void *restrict hash, u32 endianess);

/**
 * @brief      Hashes a single block of data and outputs the hash.
 *
 * @param[in]  data           Pointer to data to hash.
 * @param[in]  size           Size of the data to hash.
 * @param      hash           Pointer to memory to copy the hash to.
 * @param[in]  params         Mode and input endianess bitmask.
 * @param[in]  hashEndianess  Endianess bitmask for the hash.
 */
void sha(const void *restrict data, u32 size, void *restrict hash, u32 params, u32 hashEndianess);
