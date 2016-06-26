/**
 * 2016
 * profi200
 */

#pragma once

#include "types.h"

#define REG_NDMA_GLOBAL_CNT           *((vu32*)0x10002000)

#define REG_NDMA0_SRC_ADDR            *((vu32*)0x10002004)
#define REG_NDMA0_DST_ADDR            *((vu32*)0x10002008)
#define REG_NDMA0_TRANSFER_CNT        *((vu32*)0x1000200C)
#define REG_NDMA0_WRITE_CNT           *((vu32*)0x10002010)
#define REG_NDMA0_BLOCK_CNT           *((vu32*)0x10002014)
#define REG_NDMA0_FILL_DATA           *((vu32*)0x10002018)
#define REG_NDMA0_CNT                 *((vu32*)0x1000201C)

#define REG_NDMA1_SRC_ADDR            *((vu32*)0x10002020)
#define REG_NDMA1_DST_ADDR            *((vu32*)0x10002024)
#define REG_NDMA1_TRANSFER_CNT        *((vu32*)0x10002028)
#define REG_NDMA1_WRITE_CNT           *((vu32*)0x1000202C)
#define REG_NDMA1_BLOCK_CNT           *((vu32*)0x10002030)
#define REG_NDMA1_FILL_DATA           *((vu32*)0x10002034)
#define REG_NDMA1_CNT                 *((vu32*)0x10002038)

#define REG_NDMA2_SRC_ADDR            *((vu32*)0x1000203C)
#define REG_NDMA2_DST_ADDR            *((vu32*)0x10002040)
#define REG_NDMA2_TRANSFER_CNT        *((vu32*)0x10002044)
#define REG_NDMA2_WRITE_CNT           *((vu32*)0x10002048)
#define REG_NDMA2_BLOCK_CNT           *((vu32*)0x1000204C)
#define REG_NDMA2_FILL_DATA           *((vu32*)0x10002050)
#define REG_NDMA2_CNT                 *((vu32*)0x10002054)

#define REG_NDMA3_SRC_ADDR            *((vu32*)0x10002058)
#define REG_NDMA3_DST_ADDR            *((vu32*)0x1000205C)
#define REG_NDMA3_TRANSFER_CNT        *((vu32*)0x10002060)
#define REG_NDMA3_WRITE_CNT           *((vu32*)0x10002064)
#define REG_NDMA3_BLOCK_CNT           *((vu32*)0x10002068)
#define REG_NDMA3_FILL_DATA           *((vu32*)0x1000206C)
#define REG_NDMA3_CNT                 *((vu32*)0x10002070)

#define REG_NDMA4_SRC_ADDR            *((vu32*)0x10002074)
#define REG_NDMA4_DST_ADDR            *((vu32*)0x10002078)
#define REG_NDMA4_TRANSFER_CNT        *((vu32*)0x1000207C)
#define REG_NDMA4_WRITE_CNT           *((vu32*)0x10002080)
#define REG_NDMA4_BLOCK_CNT           *((vu32*)0x10002084)
#define REG_NDMA4_FILL_DATA           *((vu32*)0x10002088)
#define REG_NDMA4_CNT                 *((vu32*)0x1000208C)

#define REG_NDMA5_SRC_ADDR            *((vu32*)0x10002090)
#define REG_NDMA5_DST_ADDR            *((vu32*)0x10002094)
#define REG_NDMA5_TRANSFER_CNT        *((vu32*)0x10002098)
#define REG_NDMA5_WRITE_CNT           *((vu32*)0x1000209C)
#define REG_NDMA5_BLOCK_CNT           *((vu32*)0x100020A0)
#define REG_NDMA5_FILL_DATA           *((vu32*)0x100020A4)
#define REG_NDMA5_CNT                 *((vu32*)0x100020A8)

#define REG_NDMA6_SRC_ADDR            *((vu32*)0x100020AC)
#define REG_NDMA6_DST_ADDR            *((vu32*)0x100020B0)
#define REG_NDMA6_TRANSFER_CNT        *((vu32*)0x100020B4)
#define REG_NDMA6_WRITE_CNT           *((vu32*)0x100020B8)
#define REG_NDMA6_BLOCK_CNT           *((vu32*)0x100020BC)
#define REG_NDMA6_FILL_DATA           *((vu32*)0x100020C0)
#define REG_NDMA6_CNT                 *((vu32*)0x100020C4)

#define REG_NDMA7_SRC_ADDR            *((vu32*)0x100020C8)
#define REG_NDMA7_DST_ADDR            *((vu32*)0x100020CC)
#define REG_NDMA7_TRANSFER_CNT        *((vu32*)0x100020D0)
#define REG_NDMA7_WRITE_CNT           *((vu32*)0x100020D4)
#define REG_NDMA7_BLOCK_CNT           *((vu32*)0x100020D8)
#define REG_NDMA7_FILL_DATA           *((vu32*)0x100020DC)
#define REG_NDMA7_CNT                 *((vu32*)0x100020E0)

#define REG_NDMA_SRC_ADDR(n)          *((vu32*)(0x10002004+(n*28)))
#define REG_NDMA_DST_ADDR(n)          *((vu32*)(0x10002008+(n*28)))
#define REG_NDMA_TRANSFER_CNT(n)      *((vu32*)(0x1000200C+(n*28)))
#define REG_NDMA_WRITE_CNT(n)         *((vu32*)(0x10002010+(n*28)))
#define REG_NDMA_BLOCK_CNT(n)         *((vu32*)(0x10002014+(n*28)))
#define REG_NDMA_FILL_DATA(n)         *((vu32*)(0x10002018+(n*28)))
#define REG_NDMA_CNT(n)               *((vu32*)(0x1000201C+(n*28)))


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
