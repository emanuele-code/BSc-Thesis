#include <avr/interrupt.h>
#include <avr/io.h>

#include "timer.h"


/* Milliseconds since tick_init(). volatile: modified in the ISR.
 * thus, it must always be re-read from memory. */
static volatile uint32_t tick = 0;


/* ISR compare match A, fires every 1 ms 
 * increments the global millisecond counter */
ISR(TIMER1_COMPA_vect) {
    tick++;
}

/* Configure Timer1 to fire a compare match interrupt every 1 ms when TCNT1 matches OCR1A registers
 * Assumes that atmega328p has a clock frequency of 16MHz */
void tick_init() {
    OCR1A   = 249;                                        // 16MHz/64 = 250kHz (4us tick); 1ms/4us - 1 = 249
    TIMSK1 |= (1 << OCIE1A);                              // enable the output compare A match interrupt for Timer1
    TCCR1B |= (1 << WGM12) | (1 << CS11) | (1 << CS10);   // WGM12: CTC mode (reset the counter when compare match), CS11 | CS10 enable prescaler 64
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
