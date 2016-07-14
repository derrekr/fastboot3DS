#pragma once

#include "types.h"
#include "mem_map.h"



#ifdef ARM9
/* CFG */
#define CFG_REGS_BASE            (IO_MEM_ARM9_ONLY)
#define REG_CARDCONF             *((vu16*)(CFG_REGS_BASE + 0x0000C))
#define REG_CARDCONF2            *((vu8* )(CFG_REGS_BASE + 0x00010))
#define CFG_BOOTENV              *((vu32*)(CFG_REGS_BASE + 0x10000))
#define CFG_UNITINFO             *((vu8* )(CFG_REGS_BASE + 0x10010))


/* IRQ */
#define IRQ_REGS_BASE            (IO_MEM_ARM9_ONLY + 0x1000)
#define REG_IRQ_IE               *((vu32*)(IRQ_REGS_BASE + 0x00))
#define REG_IRQ_IF               *((vu32*)(IRQ_REGS_BASE + 0x04))


/* NDMA */
#define NDMA_REGS_BASE           (IO_MEM_ARM9_ONLY + 0x2000)
#define REG_NDMA_GLOBAL_CNT      *((vu32*)(NDMA_REGS_BASE + 0x00))

#define REG_NDMA0_SRC_ADDR       *((vu32*)(NDMA_REGS_BASE + 0x04))
#define REG_NDMA0_DST_ADDR       *((vu32*)(NDMA_REGS_BASE + 0x08))
#define REG_NDMA0_TRANSFER_CNT   *((vu32*)(NDMA_REGS_BASE + 0x0C))
#define REG_NDMA0_WRITE_CNT      *((vu32*)(NDMA_REGS_BASE + 0x10))
#define REG_NDMA0_BLOCK_CNT      *((vu32*)(NDMA_REGS_BASE + 0x14))
#define REG_NDMA0_FILL_DATA      *((vu32*)(NDMA_REGS_BASE + 0x18))
#define REG_NDMA0_CNT            *((vu32*)(NDMA_REGS_BASE + 0x1C))

#define REG_NDMA1_SRC_ADDR       *((vu32*)(NDMA_REGS_BASE + 0x20))
#define REG_NDMA1_DST_ADDR       *((vu32*)(NDMA_REGS_BASE + 0x24))
#define REG_NDMA1_TRANSFER_CNT   *((vu32*)(NDMA_REGS_BASE + 0x28))
#define REG_NDMA1_WRITE_CNT      *((vu32*)(NDMA_REGS_BASE + 0x2C))
#define REG_NDMA1_BLOCK_CNT      *((vu32*)(NDMA_REGS_BASE + 0x30))
#define REG_NDMA1_FILL_DATA      *((vu32*)(NDMA_REGS_BASE + 0x34))
#define REG_NDMA1_CNT            *((vu32*)(NDMA_REGS_BASE + 0x38))

#define REG_NDMA2_SRC_ADDR       *((vu32*)(NDMA_REGS_BASE + 0x3C))
#define REG_NDMA2_DST_ADDR       *((vu32*)(NDMA_REGS_BASE + 0x40))
#define REG_NDMA2_TRANSFER_CNT   *((vu32*)(NDMA_REGS_BASE + 0x44))
#define REG_NDMA2_WRITE_CNT      *((vu32*)(NDMA_REGS_BASE + 0x48))
#define REG_NDMA2_BLOCK_CNT      *((vu32*)(NDMA_REGS_BASE + 0x4C))
#define REG_NDMA2_FILL_DATA      *((vu32*)(NDMA_REGS_BASE + 0x50))
#define REG_NDMA2_CNT            *((vu32*)(NDMA_REGS_BASE + 0x54))

#define REG_NDMA3_SRC_ADDR       *((vu32*)(NDMA_REGS_BASE + 0x58))
#define REG_NDMA3_DST_ADDR       *((vu32*)(NDMA_REGS_BASE + 0x5C))
#define REG_NDMA3_TRANSFER_CNT   *((vu32*)(NDMA_REGS_BASE + 0x60))
#define REG_NDMA3_WRITE_CNT      *((vu32*)(NDMA_REGS_BASE + 0x64))
#define REG_NDMA3_BLOCK_CNT      *((vu32*)(NDMA_REGS_BASE + 0x68))
#define REG_NDMA3_FILL_DATA      *((vu32*)(NDMA_REGS_BASE + 0x6C))
#define REG_NDMA3_CNT            *((vu32*)(NDMA_REGS_BASE + 0x70))

#define REG_NDMA4_SRC_ADDR       *((vu32*)(NDMA_REGS_BASE + 0x74))
#define REG_NDMA4_DST_ADDR       *((vu32*)(NDMA_REGS_BASE + 0x78))
#define REG_NDMA4_TRANSFER_CNT   *((vu32*)(NDMA_REGS_BASE + 0x7C))
#define REG_NDMA4_WRITE_CNT      *((vu32*)(NDMA_REGS_BASE + 0x80))
#define REG_NDMA4_BLOCK_CNT      *((vu32*)(NDMA_REGS_BASE + 0x84))
#define REG_NDMA4_FILL_DATA      *((vu32*)(NDMA_REGS_BASE + 0x88))
#define REG_NDMA4_CNT            *((vu32*)(NDMA_REGS_BASE + 0x8C))

#define REG_NDMA5_SRC_ADDR       *((vu32*)(NDMA_REGS_BASE + 0x90))
#define REG_NDMA5_DST_ADDR       *((vu32*)(NDMA_REGS_BASE + 0x94))
#define REG_NDMA5_TRANSFER_CNT   *((vu32*)(NDMA_REGS_BASE + 0x98))
#define REG_NDMA5_WRITE_CNT      *((vu32*)(NDMA_REGS_BASE + 0x9C))
#define REG_NDMA5_BLOCK_CNT      *((vu32*)(NDMA_REGS_BASE + 0xA0))
#define REG_NDMA5_FILL_DATA      *((vu32*)(NDMA_REGS_BASE + 0xA4))
#define REG_NDMA5_CNT            *((vu32*)(NDMA_REGS_BASE + 0xA8))

#define REG_NDMA6_SRC_ADDR       *((vu32*)(NDMA_REGS_BASE + 0xAC))
#define REG_NDMA6_DST_ADDR       *((vu32*)(NDMA_REGS_BASE + 0xB0))
#define REG_NDMA6_TRANSFER_CNT   *((vu32*)(NDMA_REGS_BASE + 0xB4))
#define REG_NDMA6_WRITE_CNT      *((vu32*)(NDMA_REGS_BASE + 0xB8))
#define REG_NDMA6_BLOCK_CNT      *((vu32*)(NDMA_REGS_BASE + 0xBC))
#define REG_NDMA6_FILL_DATA      *((vu32*)(NDMA_REGS_BASE + 0xC0))
#define REG_NDMA6_CNT            *((vu32*)(NDMA_REGS_BASE + 0xC4))

#define REG_NDMA7_SRC_ADDR       *((vu32*)(NDMA_REGS_BASE + 0xC8))
#define REG_NDMA7_DST_ADDR       *((vu32*)(NDMA_REGS_BASE + 0xCC))
#define REG_NDMA7_TRANSFER_CNT   *((vu32*)(NDMA_REGS_BASE + 0xD0))
#define REG_NDMA7_WRITE_CNT      *((vu32*)(NDMA_REGS_BASE + 0xD4))
#define REG_NDMA7_BLOCK_CNT      *((vu32*)(NDMA_REGS_BASE + 0xD8))
#define REG_NDMA7_FILL_DATA      *((vu32*)(NDMA_REGS_BASE + 0xDC))
#define REG_NDMA7_CNT            *((vu32*)(NDMA_REGS_BASE + 0xE0))

#define REG_NDMA_SRC_ADDR(n)     *((vu32*)(NDMA_REGS_BASE + 0x04 + (n * 28)))
#define REG_NDMA_DST_ADDR(n)     *((vu32*)(NDMA_REGS_BASE + 0x08 + (n * 28)))
#define REG_NDMA_TRANSFER_CNT(n) *((vu32*)(NDMA_REGS_BASE + 0x0C + (n * 28)))
#define REG_NDMA_WRITE_CNT(n)    *((vu32*)(NDMA_REGS_BASE + 0x10 + (n * 28)))
#define REG_NDMA_BLOCK_CNT(n)    *((vu32*)(NDMA_REGS_BASE + 0x14 + (n * 28)))
#define REG_NDMA_FILL_DATA(n)    *((vu32*)(NDMA_REGS_BASE + 0x18 + (n * 28)))
#define REG_NDMA_CNT(n)          *((vu32*)(NDMA_REGS_BASE + 0x1C + (n * 28)))


/* TIMER */
#define TIMER_REGS_BASE          (IO_MEM_ARM9_ONLY + 0x3000)
#define REG_TIMER0_VAL           *((vu16*)(TIMER_REGS_BASE + 0x00))
#define REG_TIMER0_CNT           *((vu16*)(TIMER_REGS_BASE + 0x02))

#define REG_TIMER1_VAL           *((vu16*)(TIMER_REGS_BASE + 0x04))
#define REG_TIMER1_CNT           *((vu16*)(TIMER_REGS_BASE + 0x06))

#define REG_TIMER2_VAL           *((vu16*)(TIMER_REGS_BASE + 0x08))
#define REG_TIMER2_CNT           *((vu16*)(TIMER_REGS_BASE + 0x0A))

#define REG_TIMER3_VAL           *((vu16*)(TIMER_REGS_BASE + 0x0C))
#define REG_TIMER3_CNT           *((vu16*)(TIMER_REGS_BASE + 0x0E))

#define REG_TIMER_VAL(n)         *((vu16*)(TIMER_REGS_BASE + 0x00 + (n * 4)))
#define REG_TIMER_CNT(n)         *((vu16*)(TIMER_REGS_BASE + 0x02 + (n * 4)))


/* CTRCARD */
#define CTRCARD_REGS_BASE        (IO_MEM_ARM9_ONLY + 0x4000)
#define REG_CTRCARDCNT           *((vu32*)(CTRCARD_REGS_BASE + 0x00))
#define REG_CTRCARDBLKCNT        *((vu32*)(CTRCARD_REGS_BASE + 0x04))
#define REG_CTRCARDSECCNT        *((vu32*)(CTRCARD_REGS_BASE + 0x08))
#define REG_CTRCARDSECSEED       *((vu32*)(CTRCARD_REGS_BASE + 0x10))
#define REG_CTRCARDCMD            ((vu32*)(CTRCARD_REGS_BASE + 0x20))
#define REG_CTRCARDFIFO          *((vu32*)(CTRCARD_REGS_BASE + 0x30))


/* EMMC */
//#define EMMC_REGS_BASE           (IO_MEM_ARM9_ONLY + 0x6000)
// Porting the eMMC driver every time is too much so
// we leave the reg definitions in sdmmc.h


/* PXI9 */
#define PXI9_REGS_BASE           (IO_MEM_ARM9_ONLY + 0x8000)
#define REG_PXI_SYNC9            *((vu32*)(PXI9_REGS_BASE + 0x00))
#define REG_PXI_CNT9             *((vu32*)(PXI9_REGS_BASE + 0x04))
#define REG_PXI_SEND9            *((vu32*)(PXI9_REGS_BASE + 0x08))
#define REG_PXI_RECV9            *((vu32*)(PXI9_REGS_BASE + 0x0C))


/* AES */
#define AES_REGS_BASE            (IO_MEM_ARM9_ONLY + 0x9000)
#define REG_AESCNT               *((vu32*)(AES_REGS_BASE + 0x000))
#define REG_AESBLKCNT            *((vu32*)(AES_REGS_BASE + 0x004))
#define REG_AESBLKCNTH1          *((vu16*)(AES_REGS_BASE + 0x004))
#define REG_AESBLKCNTH2          *((vu16*)(AES_REGS_BASE + 0x006))
#define REG_AESWRFIFO             ((vu32*)(AES_REGS_BASE + 0x008))
#define REG_AESRDFIFO             ((vu32*)(AES_REGS_BASE + 0x00C))
#define REG_AESKEYSEL            *((vu8* )(AES_REGS_BASE + 0x010))
#define REG_AESKEYCNT            *((vu8* )(AES_REGS_BASE + 0x011))
#define REG_AESCTR                ((vu32*)(AES_REGS_BASE + 0x020))
#define REG_AESMAC                ((vu32*)(AES_REGS_BASE + 0x030))

#define REG_AESKEY0               ((vu32*)(AES_REGS_BASE + 0x040))
#define REG_AESKEYX0              ((vu32*)(AES_REGS_BASE + 0x050))
#define REG_AESKEYY0              ((vu32*)(AES_REGS_BASE + 0x060))
#define REG_AESKEY1               ((vu32*)(AES_REGS_BASE + 0x070))
#define REG_AESKEYX1              ((vu32*)(AES_REGS_BASE + 0x080))
#define REG_AESKEYY1              ((vu32*)(AES_REGS_BASE + 0x090))
#define REG_AESKEY2               ((vu32*)(AES_REGS_BASE + 0x0A0))
#define REG_AESKEYX2              ((vu32*)(AES_REGS_BASE + 0x0B0))
#define REG_AESKEYY2              ((vu32*)(AES_REGS_BASE + 0x0C0))
#define REG_AESKEY3               ((vu32*)(AES_REGS_BASE + 0x0D0))
#define REG_AESKEYX3              ((vu32*)(AES_REGS_BASE + 0x0E0))
#define REG_AESKEYY3              ((vu32*)(AES_REGS_BASE + 0x0F0))

#define REG_AESKEYFIFO            ((vu32*)(AES_REGS_BASE + 0x100))
#define REG_AESKEYXFIFO           ((vu32*)(AES_REGS_BASE + 0x104))
#define REG_AESKEYYFIFO           ((vu32*)(AES_REGS_BASE + 0x108))


/* SHA */
#define SHA_REGS_BASE            (IO_MEM_ARM9_ONLY + 0xA000)
#define REG_SHA_CNT              *((vu32*)(SHA_REGS_BASE + 0x00))
#define REG_SHA_BLKCNT           *((vu32*)(SHA_REGS_BASE + 0x04))
#define REG_SHA_HASH              ((u32* )(SHA_REGS_BASE + 0x40))
#define REG_SHA_INFIFO            (       (SHA_REGS_BASE + 0x80))


/* PRNG */
#define PRNG_REGS_BASE           (IO_MEM_ARM9_ONLY + 0x11000)
#define REG_PRNG                  ((vu32*)(PRNG_REGS_BASE))


/* OTP */
#define OTP_REGS_BASE            (IO_MEM_ARM9_ONLY + 0x12000)
#define REG_OTP                   ((vu32*)(OTP_REGS_BASE))
#endif


/* PDN */
#define PDN_REGS_BASE            (IO_MEM_ARM9_ARM11 + 0x40000)
#define REG_PDN_GPU_CNT          *((vu32*)(PDN_REGS_BASE + 0x1200))
#define REG_PDN_GPU_CNT2         *((vu32*)(PDN_REGS_BASE + 0x1204))
#define REG_PDN_GPU_CNT2_8BIT    *((vu8* )(PDN_REGS_BASE + 0x1204))
#define REG_PDN_GPU_CNT4         *((vu8* )(PDN_REGS_BASE + 0x1208))
#define REG_PDN_GPU_CNT3         *((vu16*)(PDN_REGS_BASE + 0x1210))
#define REG_PDN_MPCORE_CLKCNT    *((vu16*)(PDN_REGS_BASE + 0x1300))


#define HID_REGS_BASE            (IO_MEM_ARM9_ARM11 + 0x00046000)
#define REG_HID_PAD              ~(*((vu32*)(HID_REGS_BASE)))


/* SPI */
#define SPI_REGS_BUS2_BASE       (IO_MEM_ARM9_ARM11 + 0x60000)
#define REG_SPI_BUS2_CNT         *((vu16*)(SPI_REGS_BUS2_BASE + 0x00))
#define REG_SPI_BUS2_DATA        *((vu8* )(SPI_REGS_BUS2_BASE + 0x02))


/* PXI11 */
#define PXI11_REGS_BASE          (IO_MEM_ARM9_ARM11 + 0x63000)
#define REG_PXI_SYNC11           *((vu32*)(PXI11_REGS_BASE + 0x00))
#define REG_PXI_CNT11            *((vu32*)(PXI11_REGS_BASE + 0x04))
#define REG_PXI_SEND11           *((vu32*)(PXI11_REGS_BASE + 0x08))
#define REG_PXI_RECV11           *((vu32*)(PXI11_REGS_BASE + 0x0C))


/* NTRCARD */
#define NTRCARD_REGS_BASE        (IO_MEM_ARM9_ARM11 + 0x64000)
#define REG_NTRCARDMCNT          *((vu16*)(NTRCARD_REGS_BASE + 0x00))
#define REG_NTRCARDMDATA         *((vu16*)(NTRCARD_REGS_BASE + 0x02))
#define REG_NTRCARDROMCNT        *((vu32*)(NTRCARD_REGS_BASE + 0x04))
#define REG_NTRCARDCMD            ((vu8* )(NTRCARD_REGS_BASE + 0x08))
#define REG_NTRCARDSEEDX_L       *((vu32*)(NTRCARD_REGS_BASE + 0x10))
#define REG_NTRCARDSEEDY_L       *((vu32*)(NTRCARD_REGS_BASE + 0x14))
#define REG_NTRCARDSEEDX_H       *((vu16*)(NTRCARD_REGS_BASE + 0x18))
#define REG_NTRCARDSEEDY_H       *((vu16*)(NTRCARD_REGS_BASE + 0x1A))
#define REG_NTRCARDFIFO          *((vu32*)(NTRCARD_REGS_BASE + 0x1C))


/* I2C */
#define I2C_REGS_BUS0_BASE       (IO_MEM_ARM9_ARM11 + 0x61000)
#define I2C_REGS_BUS1_BASE       (IO_MEM_ARM9_ARM11 + 0x44000)
#define REG_I2C_BUS0_DATA        *((vu8* )(I2C_REGS_BUS0_BASE + 0x00))
#define REG_I2C_BUS1_DATA        *((vu8* )(I2C_REGS_BUS1_BASE + 0x00))
#define REG_I2C_BUS0_CNT         *((vu8* )(I2C_REGS_BUS0_BASE + 0x01))
#define REG_I2C_BUS1_CNT         *((vu8* )(I2C_REGS_BUS1_BASE + 0x01))


#ifdef ARM11
/* LCD */
#define LCD_REGS_BASE            (IO_MEM_ARM11_ONLY + 0x2000)
#define REG_LCD_COLORFILL_MAIN   *((vu32*)(LCD_REGS_BASE + 0x204))
#define REG_LCD_COLORFILL_SUB    *((vu32*)(LCD_REGS_BASE + 0xA04))
#define REG_LCD_BACKLIGHT_MAIN   *((vu32*)(LCD_REGS_BASE + 0x240))
#define REG_LCD_BACKLIGHT_SUB    *((vu32*)(LCD_REGS_BASE + 0xA40))


/* GPU External Registers */
#define GPU_EXT_REGS_BASE        (IO_MEM_ARM11_ONLY + 0x200000)
#define REG_GPU_EXT_CNT          *((vu32*)(GPU_EXT_REGS_BASE + 0x04))
#endif
