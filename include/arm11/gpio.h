#pragma once

#include "types.h"



/**
 * @brief      Sets a bit in the specified GPIO register.
 *
 * @param[in]  reg     The register number.
 * @param[in]  bitNum  The bit number.
 */
void gpioSetBit(u16 reg, u8 bitNum);

/**
 * @brief      Clears a bit in the specified GPIO register.
 *
 * @param[in]  reg     The register number.
 * @param[in]  bitNum  The bit number.
 */
void gpioClearBit(u16 reg, u8 bitNum);
