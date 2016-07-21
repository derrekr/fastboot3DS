#pragma once

#include "types.h"
#include "io.h"



#define TIMER_BASE_FREQ     (67029337.5l)

#define TIMER_COUNT_UP      (1u<<2) // For cascading at least 2 timers
#define TIMER_IRQ_ENABLE    (1u<<6)
#define TIMER_ENABLE        (1u<<7)

// Convenience macros for calculating the ticks. Based on libnds
#define TIMER_FREQ(n)       (0xFFFFu - (u16)(TIMER_BASE_FREQ / n))
#define TIMER_FREQ_64(n)    (0xFFFFu - (u16)(TIMER_BASE_FREQ / 64.0l / n))
#define TIMER_FREQ_256(n)   (0xFFFFu - (u16)(TIMER_BASE_FREQ / 256.0l / n))
#define TIMER_FREQ_1024(n)  (0xFFFFu - (u16)(TIMER_BASE_FREQ / 1024.0l / n))


typedef enum
{
	TIMER_0 = 0,
	TIMER_1 = 1,
	TIMER_2 = 2,
	TIMER_3 = 3
} Timer;

typedef enum
{
	TIMER_PRESCALER_1    = 0,
	TIMER_PRESCALER_64   = 1,
	TIMER_PRESCALER_256  = 2,
	TIMER_PRESCALER_1024 = 3
} TimerPrescaler;


void startTimer(Timer timer, TimerPrescaler prescaler, u16 ticks, bool enableIrq);
void stopTimer(Timer timer);
void _timerSleep(u32 ticks); // Use the macro instead

#define timerSleep(ms)      _timerSleep(0xFFFFFFFFu - (u32)((TIMER_BASE_FREQ / 1024.0l / 1000.0l) * ms))
