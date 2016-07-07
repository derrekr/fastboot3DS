/**
 * 2016
 * profi200
 */

#pragma once

#include "types.h"
#include "io.h"



// This bit has no effect it seems.
#define NDMA_GLOBAL_ENABLE            (1u)

#define NDMA_BLOCK_SYS_FREQ           (0u)

#define NDMA_DST_UPDATE_INC           (0u)
#define NDMA_DST_UPDATE_DEC           (1u<<10)
#define NDMA_DST_UPDATE_FIXED         (2u<<10)
#define NDMA_DST_ADDR_RELOAD          (1u<<12)
#define NDMA_SRC_UPDATE_INC           (0u)
#define NDMA_SRC_UPDATE_DEC           (1u<<13)
#define NDMA_SRC_UPDATE_FIXED         (2u<<13)
#define NDMA_SRC_UPDATE_FILL          (3u<<13)
#define NDMA_SRC_ADDR_RELOAD          (1u<<15)

// The block length is 2^n words (Example: 2^15 = 32768 words = 0x20000 bytes)
#define NDMA_BLK_TRANS_WORD_COUNT(n)  (n<<16)
#define NDMA_IMMEDIATE_MODE           (1u<<28)
#define NDMA_REPEATING_MODE           (1u<<29)
#define NDMA_INTERRUPT_ENABLE         (1u<<30)
#define NDMA_ENABLE                   (1u<<31)



/**
 * @brief      Copies data using the NDMA engine.
 *
 * @param      dest    Pointer to destination memory. Must be 4 bytes aligned.
 * @param      source  Pointer to source data. Must be 4 bytes aligned.
 * @param[in]  num     The size of the data. Must be a multiple of 4.
 */
void NDMA_copy(u32 *dest, const u32 *source, u32 num);

/**
 * @brief      Fills memory with the given value using the NDMA engine.
 *
 * @param      dest   Pointer to destination memory. Must be 4 bytes aligned.
 * @param[in]  value  The value each 32-bit word will be set to.
 * @param[in]  num    The size of the memory to fill. Must be a multiple of 4.
 */
void NDMA_fill(u32 *dest, u32 value, u32 num);
