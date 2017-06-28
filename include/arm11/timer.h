#pragma once

#include "types.h"


#define TIMER_BASE_FREQ    (268111856.0)

#define TIMER_ENABLE       (1u)
#define TIMER_SINGLE_SHOT  (0u)
#define TIMER_AUTO_RELOAD  (1u<<1)
#define TIMER_IRQ_ENABLE   (1u<<2)

// p is the prescaler value and n the frequence
#define TIMER_FREQ(p, n)   (TIMER_BASE_FREQ / 2 / ((p) + 1) / (n))



/**
 * @brief      Resets/initializes the timer hardware. Should not be called manually.
 */
void TIMER_init(void);

/**
 * @brief      Starts the timer.
 *
 * @param[in]  prescaler   The prescaler value.
 * @param[in]  ticks       The initial number of ticks. This is also the
 *                         reload value in auto reload mode.
 * @param[in]  autoReload  Set to true for auto reload. false for single shot.
 * @param[in]  enableIrq   Timer fires IRQs on underflow if true.
 */
void TIMER_start(u8 prescaler, u32 ticks, bool autoReload, bool enableIrq);

/**
 * @brief      Stops the timer.
 */
void TIMER_stop(void);

/**
 * @brief      Halts the CPU for the specified number of ticks.
 *             Use the macro below for a milliseconds version.
 *
 * @param[in]  ticks  The number of ticks to sleep.
 */
void TIMER_sleepTicks(u32 ticks);

// Sleeps ms milliseconds. ms can be up to 32000.
#define TIMER_sleepMs(ms)  TIMER_sleepTicks(TIMER_FREQ(0, 1000.0) * (ms))
