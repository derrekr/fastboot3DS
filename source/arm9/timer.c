#include "types.h"
#include "arm9/timer.h"
#include "arm9/interrupt.h"



void startTimer(Timer timer, TimerPrescaler prescaler, u16 ticks, bool enableIrq)
{
	REG_TIMER_VAL(timer) = ticks;

	if(enableIrq) REG_IRQ_IE |= ((u32)INTERRUPT_TIMER_0<<timer);

	REG_TIMER_CNT(timer) = TIMER_ENABLE | ((u32)enableIrq<<6) | prescaler;
}

void stopTimer(Timer timer)
{
	REG_IRQ_IE &= ~((u32)INTERRUPT_TIMER_0<<timer);
	REG_IRQ_IF = ((u32)INTERRUPT_TIMER_0<<timer);
	REG_TIMER_CNT(timer) = 0;
}

void timerSleep(u32 ms)
{
	u32 tmp = 0xFFFFFFFFu - (u32)((((double)TIMER_BASE_FREQ / 1024.0f) / 1000.0f) * ms);

	REG_TIMER2_VAL = (u16)tmp;
	REG_TIMER3_VAL = (u16)(tmp>>16);

	REG_IRQ_IE |= (u32)INTERRUPT_TIMER_3;

	REG_TIMER3_CNT = TIMER_ENABLE | TIMER_INTERRUPT_ENABLE | TIMER_COUNT_UP;
	REG_TIMER2_CNT = TIMER_ENABLE | TIMER_PRESCALER_1024;

	//while(!(REG_IRQ_IF & (1u<<11)))
	//{
		__asm__("mcr p15, 0, %[in], c7, c0, 4\n" : : [in] "r" (0)); // Wait for interrupt
	//}
	REG_IRQ_IF = (u32)INTERRUPT_TIMER_3;
	REG_IRQ_IE &= ~((u32)INTERRUPT_TIMER_3);

	REG_TIMER2_CNT = REG_TIMER3_CNT = 0;
}

