#pragma once

#include "types.h"
#include "io.h"

// Defines for PX_SYNC regs
#define PXI_DATA_RECEIVED(reg)      (reg & 0xFF)
#define PXI_DATA_SENT(reg)          (reg>>8 & 0xFF)
#define PXI_NOTIFY_11               (1u<<29)
#define PXI_NOTIFY_9                (1u<<30)
#define PXI_IRQ_ENABLE              (1u<<31)

// Defines for PX_CNT regs
#define PXI_SEND_FIFO_EMPTY         (1u<<0)
#define PXI_SEND_FIFO_FULL          (1u<<1)
#define PXI_SEND_FIFO_EMPTY_IRQ     (1u<<2)
#define PXI_FLUSH_SEND_FIFO         (1u<<3)
#define PXI_RECV_FIFO_EMPTY         (1u<<8)
#define PXI_RECV_FIFO_FULL          (1u<<9)
#define PXI_RECV_FIFO_NOT_EMPTY_IRQ (1u<<10)
#define PXI_EMPTY_FULL_ERROR        (1u<<14)
#define PXI_ENABLE_SEND_RECV_FIFO   (1u<<15)

// Custom PXI Command/Reply Definitions
#define PXI_CMD_ENABLE_LCDS			0x4C434453
#define PXI_CMD_POWER_OFF			0x504F4646
#define PXI_CMD_ALLOW_POWER_OFF		0x504F4F4B
#define PXI_CMD_FORBID_POWER_OFF	0x504F4E4F
#define PXI_CMD_FIRM_LAUNCH			0x544F4F42
#define PXI_RPL_FIRM_LAUNCH_READY	0x4F4B6666
#define PXI_RPL_OK					0x4F4B4F4B
#define PXI_RPL_HOME_PRESSED		0x484F4D45
#define PXI_RPL_POWER_PRESSED		0x504F5752


void PXI_init(void);
void PXI_sendWord(u32 val);
bool PXI_trySendWord(u32 val);
u32  PXI_recvWord(void);
u32 PXI_tryRecvWord(bool *success);
