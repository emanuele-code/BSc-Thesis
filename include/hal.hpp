/* hal.hpp
 *
 * DESCRIPTION
 * Hardware layer of the library: one include to get the board support.
 *
 * WHAT IT DOES
 * Includes utilitySerial.hpp (UART0) and utilityPin.hpp (digital pins), so that
 * the user only needs "hal.hpp". It declares hal_init(), which starts the hardware
 * that the library needs.
 *
 * LIMITS
 * hal_init() must be called first in main, before thread_sleep or tick_now are used
 * (the tick starts there) and before the threads are created.
 */



#pragma once

#include "utilitySerial.hpp"
#include "utilityPin.hpp"





void hal_init();       