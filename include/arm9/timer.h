#pragma once

#include "types.h"


#define TIMER_BASE_FREQ     (67027964.0)

#define TIMER_COUNT_UP      (1u<<2) // For cascading at least 2 timers
#define TIMER_IRQ_ENABLE    (1u<<6)
#define TIMER_ENABLE        (1u<<7)

// Convenience macros for calculating the ticks. Based on libnds
#define TIMER_FREQ(n)       (0xFFFFu - (u16)(TIMER_BASE_FREQ / n) + 1)
#define TIMER_FREQ_64(n)    (0xFFFFu - (u16)(TIMER_BASE_FREQ / 64.0 / n) + 1)
#define TIMER_FREQ_256(n)   (0xFFFFu - (u16)(TIMER_BASE_FREQ / 256.0 / n) + 1)
#define TIMER_FREQ_1024(n)  (0xFFFFu - (u16)(TIMER_BASE_FREQ / 1024.0 / n) + 1)


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



/**
 * @brief      Resets/initializes the timer hardware. Should not be called manually.
 */
void TIMER_init(void);

/**
 * @brief      Starts a timer.
 *
 * @param[in]  timer       The timer to start.
 * @param[in]  prescaler   The prescaler to use.
 * @param[in]  ticks       The initial number of ticks. This is also the reload
 *                         value on overflow.
 * @param[in]  irqHandler  The IRQ handler function for this timer. NULL to disable IRQ.
 */
void TIMER_start(Timer timer, TimerPrescaler prescaler, u16 ticks, void (*irqHandler)(void));

/**
 * @brief      Stops a timer.
 *
 * @param[in]  timer  The timer to stop.
 */
void TIMER_stop(Timer timer);

/**
 * @brief      Halts the processor for the specified number of ticks. Use the macro below.
 *
 * @param[in]  ticks  The number of ticks.
 */
void _timerSleep(u32 ticks);

#define TIMER_sleep(ms)      _timerSleep(0xFFFFFFFFu - (u32)((TIMER_BASE_FREQ / 1024.0 / 1000.0) * ms) + 1)
