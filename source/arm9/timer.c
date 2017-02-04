#include "types.h"
#include "arm9/timer.h"
#include "arm9/interrupt.h"



void TIMER_init(void)
{
	u32 oldState = enterCriticalSection();

	for(u32 i = 0; i < 4; i++)
	{
		REG_TIMER_CNT(i) = 0;
	}

	REG_IRQ_IE |= ((1u<<IRQ_TIMER_0) | (1u<<IRQ_TIMER_1) | (1u<<IRQ_TIMER_2) | (1u<<IRQ_TIMER_3));

	leaveCriticalSection(oldState);
}

void TIMER_start(Timer timer, TimerPrescaler prescaler, u16 ticks, void (*irqHandler)(void))
{
	u16 irq = 0;

	if(irqHandler)
	{
		IRQ_registerHandler(IRQ_TIMER_0 + timer, irqHandler);
		irq = TIMER_IRQ_ENABLE;
	}

	REG_TIMER_VAL(timer) = ticks;
	REG_TIMER_CNT(timer) = TIMER_ENABLE | irq | prescaler;
}

void TIMER_stop(Timer timer)
{
	REG_TIMER_CNT(timer) = 0;
	IRQ_unregisterHandler(IRQ_TIMER_0 + timer);
}

static void timerSleepHandler(void)
{
	REG_TIMER3_CNT = REG_TIMER2_CNT = 0;
	IRQ_unregisterHandler(IRQ_TIMER_3);
}

void _timerSleep(u32 ticks)
{
	REG_TIMER2_VAL = (u16)ticks;
	REG_TIMER3_VAL = (u16)(ticks>>16);

	IRQ_registerHandler(IRQ_TIMER_3, timerSleepHandler);

	REG_TIMER3_CNT = TIMER_ENABLE | TIMER_IRQ_ENABLE | TIMER_COUNT_UP;
	REG_TIMER2_CNT = TIMER_ENABLE | TIMER_PRESCALER_1024;

	while(REG_TIMER3_CNT & TIMER_ENABLE)
	{
		waitForIrq();
	}
}
