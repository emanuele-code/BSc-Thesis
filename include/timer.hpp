/* timer.hpp
 *
 * DESCRIPTION
 * Interface of the millisecond tick (implementation in timer.cc).
 *
 * WHAT IT DOES
 * tick_init() starts the tick (Timer0, one interrupt every 1 ms) and tick_now() returns
 * the milliseconds elapsed since then. The counter is 32 bit: it wraps around after
 * about 49.7 days.
 *
 * LIMITS
 * - Subtract two values of tick_now() as uint32_t (end - start): the result is correct
 *   even when the counter wraps around
 * - The resolution is 1 ms
 */



#pragma once

#include <stdint.h>





void     tick_init();      // start the 1 ms tick: called by hal_init, interrupts must be enabled after it
uint32_t tick_now();       // milliseconds since tick_init (atomic read: interrupts are disabled for a moment)