#pragma once



/* PDN */
#define PDN_REGS_BASE				0x10140000
#define PDN_REG_GPU_CNT				*((vu32 *)(PDN_REGS_BASE+0x1200))
#define PDN_REG_GPU_CNT2			*((vu32 *)(PDN_REGS_BASE+0x1204))
#define PDN_REG_GPU_CNT2_8BIT		*((vu8 *)(PDN_REGS_BASE+0x1204))
#define PDN_REG_GPU_CNT4			*((vu8 *)(PDN_REGS_BASE+0x1208))
#define PDN_REG_GPU_CNT3			*((vu16 *)(PDN_REGS_BASE+0x1210))

/* I2C */
#define I2C_REGS_BUS0_BASE			0x10161000
#define I2C_REGS_BUS1_BASE			0x10144000
#define I2C_REGS_BUS0_DATA			*((vu8 *)(I2C_REGS_BUS0_BASE))
#define I2C_REGS_BUS1_DATA			*((vu8 *)(I2C_REGS_BUS1_BASE))
#define I2C_REGS_BUS0_CNT			*((vu8 *)(I2C_REGS_BUS0_BASE+0x01))
#define I2C_REGS_BUS1_CNT			*((vu8 *)(I2C_REGS_BUS1_BASE+0x01))

/* PXI */
#define PXI_SYNC9					*((vu32 *)(0x10008000))
#define PXI_SYNC11					*((vu32 *)(0x10163000))

/* LCD */
#define LCD_REGS_BASE				0x10202000
#define LCD_REG_LCDCOLORFILLMAIN	*((vu32 *)(LCD_REGS_BASE+0x204))
#define LCD_REG_LCDCOLORFILLSUB		*((vu32 *)(LCD_REGS_BASE+0xA04))
#define LCD_REG_BACKLIGHTMAIN		*((vu32 *)(LCD_REGS_BASE+0x240))
#define LCD_REG_BACKLIGHTSUB		*((vu32 *)(LCD_REGS_BASE+0xA40))

/* GPU External Registers */
#define GPU_EXT_REGS_BASE			0x10400000
#define GPU_EXT_REG_CNT				*((vu32 *)(GPU_EXT_REGS_BASE+0x4))
