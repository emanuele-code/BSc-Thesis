#include <avr/interrupt.h>
#include <avr/io.h>

#include "timer.h"


/* Milliseconds since tick_init(). volatile: modified in the ISR.
 * thus, it must always be re-read from memory. */
static volatile uint32_t tick = 0;


/* ISR compare match A, fires every 1 ms
 * increments the global millisecond counter */
ISR(TIMER0_COMPA_vect) {
    tick++;
}

/* Configure Timer0 to fire a compare match interrupt every 1 ms when TCNT0 matches OCR0A registers
 * Assumes that atmega328p has a clock frequency of 16MHz */
void tick_init() {
    OCR0A   = 249;                                        // 16MHz/64 = 250kHz (4us tick); 1ms/4us - 1 = 249
    TIMSK0 |= (1 << OCIE0A);                              // enable the output compare A match interrupt for Timer0
    TCCR0A |= (1 << WGM01);                               // WGM01: CTC mode (reset the counter on compare match)
    TCCR0B |= (1 << CS01) | (1 << CS00);                  // CS01 | CS00: enable prescaler 64
}

/* the AVR is an 8-bit MCU but the tick is 32 bit (split on 4 registers)
 * so it could be interrupted amid the reading process leading to a dirty read
 * thus, to make the read atomic during the copy interrupts are disabled */
uint32_t tick_now() {
    uint8_t  sreg = SREG;           // copy status register
    cli();                          // disable I-bit (global interrupt enable) of SREG register
    uint32_t temp_tick = tick;
    SREG = sreg;
    return temp_tick;
}
