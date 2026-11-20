/* main.cc
 *
 * DESCRIPTION
 * Entry point of the program: it initializes the hardware, lets the user code
 * create the threads and then becomes the idle thread.
 *
 * WHAT IT DOES
 * Starts the millisecond tick and the UART, enables the interrupts and calls
 * setup(), where the user creates its threads in the file userSetup.cc. After that,
 * main keeps giving the CPU to the other threads: main is itself a thread of the
 * round robin (tcb_main), so the scheduler starts at its first thread_yield().
 *
 * LIMITS
 * main must never return: the program would end and all the threads with it
 */



#include <avr/interrupt.h>

#include "utilitySerial.hpp"
#include "userSetup.hpp"
#include "thread.hpp"
#include "timer.hpp"




int main(void) {
    tick_init();                // Timer0: 1 ms tick (does not enable the interrupts)
    uart_init();                // UART0 at 9600 baud, used for debugging
    sei();                      // enable interrupts

    setup();                    // user code

    for(;;) thread_yield();     // idle: give the CPU to the other threads, forever
}