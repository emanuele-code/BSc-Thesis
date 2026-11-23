/* hal.cc
 *
 * DESCRIPTION
 * Implementation of hal_init, declared in hal.hpp.
 *
 * WHAT IT DOES
 * Starts the 1 ms tick (Timer0), configures the UART0 used for debugging and
 * enables the interrupts.
 *
 * LIMITS
 * Interrupts are enabled at the end: everything that must be ready before the first
 * tick has to be done before this function returns.
 */



#include <avr/interrupt.h>

#include "hal.hpp"
#include "timer.hpp"
#include "utilitySerial.hpp"





void hal_init() {
    tick_init();       // Timer0: 1 ms tick
    uart_init();       // UART0 at 9600 baud, used for debugging

    sei();             // enable interrupts
}