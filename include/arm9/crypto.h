/*
 *  2017
 *  profi200
 */

#pragma once

#include "types.h"



//////////////////////////////////
//             AES              //
//////////////////////////////////

#define AES_MAX_BLOCKS        (0xFFFF)

#define AES_WRITE_FIFO_COUNT  (REG_AESCNT & 0x1F)
#define AES_READ_FIFO_COUNT   (REG_AESCNT & 0x3E0)

#define AES_FLUSH_READ_FIFO   (1u<<10)
#define AES_FLUSH_WRITE_FIFO  (1u<<11)
#define AES_MAC_SIZE(n)       (((n - 2) / 2)<<16)
#define AES_PASS_PAYLOARD     (1u<<19) // Passes the associated data to REG_AESRDFIFO
#define AES_MAC_SRC_REG       (1u<<20)
#define AES_IS_MAC_VALID      ((bool)(REG_AESCNT>>21 & 1u))

#define AES_OUTPUT_BIG        (1u)
#define AES_OUTPUT_LITTLE     (0u)
#define AES_INPUT_BIG         (1u)
#define AES_INPUT_LITTLE      (0u)
#define AES_OUTPUT_NORMAL     (4u)
#define AES_OUTPUT_REVERSED   (0u)
#define AES_INPUT_NORMAL      (4u)
#define AES_INPUT_REVERSED    (0u)

#define AES_UPDATE_KEYSLOT    (1u<<26)
#define AES_IRQ_ENABLE        (1u<<30)
#define AES_ENABLE            (1u<<31)

#define AES_MODE_CCM_DECRYPT  (0u)
#define AES_MODE_CCM_ENCRYPT  (1u<<27)
#define AES_MODE_CTR          (2u<<27)
#define AES_MODE_CBC_DECRYPT  (4u<<27)
#define AES_MODE_CBC_ENCRYPT  (5u<<27)
#define AES_MODE_ECB_DECRYPT  (6u<<27)
#define AES_MODE_ECB_ENCRYPT  (7u<<27)


typedef enum
{
	AES_KEY_NORMAL = 0,
	AES_KEY_X      = 1,
	AES_KEY_Y      = 2,
} AesKeyType;

typedef struct
{
	u32 ctrIvNonceParams;
	u32 ctrIvNonce[4];
	u32 aesParams;
} AES_ctx;


/**
 * @brief      Initializes the AES hardware and the NDMA channels used by it.
 */
void AES_init(void);

/**
 * @brief      Deinits AES to workaround a K9L bug.
 */
void AES_deinit(void);

/**
 * @brief      Sets a AES key in the specified keyslot.
 *
 * @param[in]  keyslot         The keyslot this key will be set for.
 * @param[in]  type            The key type. Can be AES_KEY_NORMAL/X/Y.
 * @param[in]  orderEndianess  Word order and endianess bitmask.
 * @param[in]  twlScrambler    Set to true to use the TWL keyscrambler for keyslots > 0x03.
 * @param[in]  key             Pointer to 128-bit AES key data.
 */
void AES_setKey(u8 keyslot, AesKeyType type, u8 orderEndianess, bool twlScrambler, const u32 key[4]);

/**
 * @brief      Selects the given keyslot for all following crypto operations.
 *
 * @param[in]  keyslot  The keyslot to select.
 */
void AES_selectKeyslot(u8 keyslot);

/**
 * @brief      Copies the given nonce into internal state.
 *
 * @param      ctx             Pointer to AES_ctx (AES context).
 * @param[in]  orderEndianess  Word order and endianess bitmask.
 * @param[in]  nonce           Pointer to the nonce data.
 */
void AES_setNonce(AES_ctx *const ctx, u8 orderEndianess, const u32 nonce[3]);

/**
 * @brief      Copies the given counter/initialization vector into internal state.
 *
 * @param      ctx             Pointer to AES_ctx (AES context).
 * @param[in]  orderEndianess  Word order and endianess bitmask.
 * @param[in]  ctrIv           Pointer to the counter/initialization vector data.
 */
void AES_setCtrIv(AES_ctx *const ctx, u8 orderEndianess, const u32 ctrIv[4]);

/**
 * @brief      Increments the internal counter with the given value (CTR mode).
 *
 * @param      ctr   Pointer to the counter data.
 * @param[in]  val   Value to increment the counter with.
 */
void AES_addCounter(u32 ctr[4], u32 val);

/**
 * @brief      Decrements the internal counter with the given value (CTR mode).
 *
 * @param      ctr   Pointer to the counter data.
 * @param[in]  val   Value to decrement the counter with.
 */
void AES_subCounter(u32 ctr[4], u32 val);

/**
 * @brief      Sets params in the AES context for all following crypto operations.
 *
 * @param      ctx                Pointer to AES_ctx (AES context).
 * @param[in]  inEndianessOrder   Input endianess and word order bitmask.
 * @param[in]  outEndianessOrder  Output endianess and word order bitmask.
 */
void AES_setCryptParams(AES_ctx *const ctx, u8 inEndianessOrder, u8 outEndianessOrder);

/**
 * @brief      En-/decrypts data with AES CTR.
 *
 * @param      ctx     Pointer to AES_ctx (AES context).
 * @param[in]  in      In data pointer. Can be the same as out.
 * @param      out     Out data pointer. Can be the same as in.
 * @param[in]  blocks  Number of blocks to process. 1 block is 16 bytes.
 * @param[in]  dma     Set to true to enable DMA.
 */
void AES_ctr(AES_ctx *const ctx, const u32 *in, u32 *out, u32 blocks, bool dma);

/**
 * @brief      En-/decrypts data with AES ECB.
 *
 * @param      ctx     Pointer to AES_ctx (AES context).
 * @param[in]  in      In data pointer. Can be the same as out.
 * @param      out     Out data pointer. Can be the same as in.
 * @param[in]  blocks  Number of blocks to process. 1 block is 16 bytes.
 * @param[in]  enc     Set to true to encrypt and false to decrypt.
 * @param[in]  dma     Set to true to enable DMA.
 */
void AES_ecb(AES_ctx *const ctx, const u32 *in, u32 *out, u32 blocks, bool enc, bool dma);

/**
 * @brief      En-/decrypts data with AES CCM.
 * @brief      Note: The AES hardware implements this in a non-standard way
 * @brief      limiting it to 1 nonce for 1 MB.
 *
 * @param      ctx      Pointer to AES_ctx (AES context).
 * @param[in]  in       In data pointer. Can be the same as out.
 * @param      out      Out data pointer. Can be the same as in.
 * @param[in]  macSize  The AES MAC size in bytes.
 * @param      mac      Pointer to in/out AES MAC. The MAC must/will be padded
 *                      with zeros (non-standard).
 * @param[in]  blocks   Number of blocks to process. 1 block is 16 bytes.
 * @param[in]  enc      Set to true to encrypt and false to decrypt.
 *
 * @return     Returns true in decryption mode if the AES MAC is valid. Otherwise true.
 */
bool AES_ccm(const AES_ctx *const ctx, const u32 *const in, u32 *const out, u32 macSize,
             u32 mac[4], u16 blocks, bool enc);



//////////////////////////////////
//             SHA              //
//////////////////////////////////

#define SHA_ENABLE         (1u) // Also used as busy flag
#define SHA_PAD_INPUT      (1u<<1)
#define SHA_INPUT_BIG      (1u<<3)
#define SHA_INPUT_LITTLE   (0u)
#define SHA_OUTPUT_BIG     (SHA_INPUT_BIG)
#define SHA_OUTPUT_LITTLE  (SHA_INPUT_LITTLE)
#define SHA_MODE_256       (0u)
#define SHA_MODE_224       (1u<<4)
#define SHA_MODE_1         (2u<<4)


/**
 * @brief      Sets input mode, endianess and starts the hash operation.
 *
 * @param[in]  params  Mode and input endianess bitmask.
 */
void SHA_start(u8 params);

/**
 * @brief      Hashes the data pointed to.
 *
 * @param[in]  data  Pointer to data to hash.
 * @param[in]  size  Size of the data to hash.
 */
void SHA_update(const u32 *data, u32 size);

/**
 * @brief      Generates the final hash.
 *
 * @param      hash       Pointer to memory to copy the hash to.
 * @param[in]  endianess  Endianess bitmask for the hash.
 */
void SHA_finish(u32 *const hash, u8 endianess);

/**
 * @brief      Hashes a single block of data and outputs the hash.
 *
 * @param[in]  data           Pointer to data to hash.
 * @param[in]  size           Size of the data to hash.
 * @param      hash           Pointer to memory to copy the hash to.
 * @param[in]  params         Mode and input endianess bitmask.
 * @param[in]  hashEndianess  Endianess bitmask for the hash.
 */
void sha(const u32 *data, u32 size, u32 *const hash, u8 params, u8 hashEndianess);
