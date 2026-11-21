#include <avr/interrupt.h>

#include "hal.hpp"
#include "timer.hpp"
#include "utilitySerial.hpp"





void hal_init() {
    tick_init();       // Timer0: 1 ms tick
    uart_init();       // UART0 at 9600 baud, used for debugging

    sei();             // enable interrupts
}