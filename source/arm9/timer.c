#include "types.h"
#include "arm9/timer.h"
#include "arm9/interrupt.h"



void startTimer(Timer timer, TimerPrescaler prescaler, u16 ticks, bool enableIrq)
{
	REG_TIMER_VAL(timer) = ticks;
	REG_TIMER_CNT(timer) = TIMER_ENABLE | ((u32)enableIrq<<6) | prescaler;
}

void stopTimer(Timer timer)
{
	REG_TIMER_CNT(timer) = 0;
	REG_IRQ_IF = ((u32)IRQ_TIMER_0<<timer);
}

void _timerSleep(u32 ticks)
{
	REG_TIMER2_VAL = (u16)ticks;
	REG_TIMER3_VAL = (u16)(ticks>>16);

	REG_TIMER3_CNT = TIMER_ENABLE | TIMER_IRQ_ENABLE | TIMER_COUNT_UP;
	REG_TIMER2_CNT = TIMER_ENABLE | TIMER_PRESCALER_1024;

	while(!(REG_IRQ_IF & (u32)IRQ_TIMER_3))
	{
		waitForIrq();
	}

	REG_TIMER2_CNT = REG_TIMER3_CNT = 0;
	REG_IRQ_IF = (u32)IRQ_TIMER_3;
}

