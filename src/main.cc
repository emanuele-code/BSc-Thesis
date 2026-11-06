#include <avr/interrupt.h>

#include "utilityPin.h"
#include "timer.h"



/* It blinks the LED on pin 13 at 1Hz (toggle every 500ms)
 * timed by the 1ms tick from timer.h instead of a blocking delay */
int main(void) {
    DP13::output();                      // LED pin as output (without evalueting this pin we will not get 5V when PORTB is evaluated)
        
    tick_init();                         // configure Timer0 (1 ms tick)
    sei();                               // enable global interrupts

    uint32_t last = 0;                   // tick value at the last toggle
    for(;;) {             
        uint32_t now = tick_now();       // safe copy of the tick counter
        if( now - last >= 500  ) {       // 500 ms elapsed?
            last = now;
            DP13::toggle();
        }
    }
}



















