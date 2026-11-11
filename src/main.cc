#include <avr/interrupt.h>
#include <util/delay.h>

#include "utilityPin.hpp"
#include "timer.hpp"
#include "protothread.hpp"



struct Task {
    Protothread pt;
    uint32_t    last_tick;
    uint32_t    i;
};


/* This function just toggle a LED with protothreads 
 * PIN      pin to toggle 
 * ITER     number of times you want to toggle (2xITER to alternate on/off state)  
 * PERIOD   ms to wait before a toggle
 * This code will be deleted afterwards but held as a reference in github commits */
template <typename PIN, uint32_t ITER, uint16_t PERIOD>
int blink(Task *t) {
    uint32_t now = tick_now();

    PT_BEGIN(&t->pt);

    for(;;) {
        for(t->i = 0; t->i < 2*ITER; t->i++) {
            PIN::toggle();
            t->last_tick = now;
            PT_WAIT_UNTIL(&t->pt, now - t->last_tick >= PERIOD);
        }
        PT_YIELD(&t->pt);
    }

    PT_END(&t->pt);
}




int main(void) {
    DP11::output();
    DP12::output();
    DP13::output();

    tick_init();
    sei();

    Task tasks[3] = {
        {0, 0}, 
        {0, 0},
        {0, 0}
    };
    
    for(;;) {
        while( blink<DP11, 3, 500>(&tasks[0]) != PT_YIELDED ) { }
        while( blink<DP12, 2, 500>(&tasks[1]) != PT_YIELDED ) { }
        while( blink<DP13, 1, 500>(&tasks[2]) != PT_YIELDED ) { }
    }
}
