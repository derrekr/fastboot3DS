#include "types.h"
#include "mem_map.h"
#include "arm11/timer.h"
#include "arm11/interrupt.h"

#define TIMER_REGS_BASE     (MPCORE_PRIV_REG_BASE + 0x600)
#define REG_TIMER_LOAD      *((vu32*)(TIMER_REGS_BASE + 0x00))
#define REG_TIMER_COUNTER   *((vu32*)(TIMER_REGS_BASE + 0x04))
#define REG_TIMER_CNT       *((vu32*)(TIMER_REGS_BASE + 0x08))
#define REG_TIMER_INT_STAT  *((vu32*)(TIMER_REGS_BASE + 0x0C))



void TIMER_init(void)
{
	REG_TIMER_CNT = 0;
	REG_TIMER_INT_STAT = 1;

	IRQ_registerHandler(IRQ_TIMER, 12, 0, true, NULL);
}

void TIMER_start(u8 prescaler, u32 ticks, bool autoReload, bool enableIrq)
{
	REG_TIMER_LOAD = ticks;
	REG_TIMER_CNT = prescaler<<8 | (enableIrq ? TIMER_IRQ_ENABLE : 0) |
	                (autoReload ? TIMER_AUTO_RELOAD : TIMER_SINGLE_SHOT) | TIMER_ENABLE;
}

void TIMER_stop(void)
{
	REG_TIMER_CNT = 0;
	REG_TIMER_INT_STAT = 1;
}

void TIMER_sleepTicks(u32 ticks)
{
	REG_TIMER_LOAD = ticks;
	REG_TIMER_CNT = 0u<<8 | TIMER_IRQ_ENABLE | TIMER_SINGLE_SHOT | TIMER_ENABLE;

	while(REG_TIMER_COUNTER)
	{
		waitForInterrupt();
	}

	REG_TIMER_INT_STAT = 1;
}
