#pragma once

#include "types.h"
#include "io.h"



#define TIMER_BASE_FREQ        (67029337.5l)

#define TIMER_COUNT_UP         (1u<<2) // For cascading at least 2 timers
#define TIMER_INTERRUPT_ENABLE (1u<<6)
#define TIMER_ENABLE           (1u<<7)

// Convenience macros for calculating the ticks. Based on libnds
#define TIMER_FREQ(n)          (0xFFFFu - (u16)(TIMER_BASE_FREQ / n))
#define TIMER_FREQ_64(n)       (0xFFFFu - (u16)(TIMER_BASE_FREQ / 64.0l / n))
#define TIMER_FREQ_256(n)      (0xFFFFu - (u16)(TIMER_BASE_FREQ / 256.0l / n))
#define TIMER_FREQ_1024(n)     (0xFFFFu - (u16)(TIMER_BASE_FREQ / 1024.0l / n))


typedef enum
{
	TIMER_0 = 0u,
	TIMER_1 = 1u,
	TIMER_2 = 2u,
	TIMER_3 = 3u
} Timer;

typedef enum
{
	TIMER_PRESCALER_1    = 0u,
	TIMER_PRESCALER_64   = 1u,
	TIMER_PRESCALER_256  = 2u,
	TIMER_PRESCALER_1024 = 3u
} TimerPrescaler;


void startTimer(Timer timer, TimerPrescaler prescaler, u16 ticks, bool enableIrq);
void stopTimer(Timer timer);
void timerSleep(u32 ms);
