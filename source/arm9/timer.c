#include "mem_map.h"
#include "types.h"
#include "arm9/timer.h"
#include "arm9/interrupt.h"


#define TIMER_REGS_BASE   (IO_MEM_ARM9_ONLY + 0x3000)
#define REG_TIMER0_VAL    *((vu16*)(TIMER_REGS_BASE + 0x00))
#define REG_TIMER0_CNT    *((vu16*)(TIMER_REGS_BASE + 0x02))

#define REG_TIMER1_VAL    *((vu16*)(TIMER_REGS_BASE + 0x04))
#define REG_TIMER1_CNT    *((vu16*)(TIMER_REGS_BASE + 0x06))

#define REG_TIMER2_VAL    *((vu16*)(TIMER_REGS_BASE + 0x08))
#define REG_TIMER2_CNT    *((vu16*)(TIMER_REGS_BASE + 0x0A))

#define REG_TIMER3_VAL    *((vu16*)(TIMER_REGS_BASE + 0x0C))
#define REG_TIMER3_CNT    *((vu16*)(TIMER_REGS_BASE + 0x0E))

#define REG_TIMER_VAL(n)  *((vu16*)(TIMER_REGS_BASE + 0x00 + (n * 4)))
#define REG_TIMER_CNT(n)  *((vu16*)(TIMER_REGS_BASE + 0x02 + (n * 4)))


// For TIMER_sleep()
static u32 overflows;



void TIMER_init(void)
{
	for(u32 i = 0; i < 4; i++)
	{
		REG_TIMER_CNT(i) = 0;
	}

	REG_IRQ_IE |= 1u<<IRQ_TIMER_3;
}

void TIMER_start(Timer timer, TimerPrescaler prescaler, u16 ticks, bool enableIrq)
{
	REG_TIMER_VAL(timer) = ticks;
	REG_TIMER_CNT(timer) = TIMER_ENABLE | (enableIrq ? TIMER_IRQ_ENABLE : 0) | prescaler;
}

void TIMER_stop(Timer timer)
{
	REG_TIMER_CNT(timer) = 0;
}

static void timerSleepHandler(void)
{
	overflows--;
	if(!overflows)
	{
		REG_TIMER3_CNT = 0;
		IRQ_unregisterHandler(IRQ_TIMER_3);
	}
}

void TIMER_sleep(u32 ms)
{
	REG_TIMER3_VAL = TIMER_FREQ_1024(1000.0);
	overflows = ms;

	IRQ_registerHandler(IRQ_TIMER_3, timerSleepHandler);

	REG_TIMER3_CNT = TIMER_ENABLE | TIMER_IRQ_ENABLE | TIMER_PRESCALER_1024;

	while(REG_TIMER3_CNT & TIMER_ENABLE)
	{
		waitForIrq();
	}
}
