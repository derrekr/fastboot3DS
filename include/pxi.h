#pragma once

#include "types.h"
#include "io.h"



// Defines for PX_SYNC regs
#define PXI_DATA_RECEIVED(reg)      (reg & 0xFF)
#define PXI_DATA_SENT(reg)          ((reg>>8) & 0xFF)
#define PXI_NOTIFY_11               (1u<<29)
#define PXI_NOTIFY_9                (1u<<30)
#define PXI_INTERRUPT_ENABLE        (1u<<31)

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


void PXI_init(void);
void PXI_sendWord(u32 val);
u32  PXI_recvWord(void);
