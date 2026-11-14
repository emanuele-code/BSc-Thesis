#include <avr/interrupt.h>
#include <stdint.h>

#include "utilitySerial.hpp"
#include "utilityPin.hpp"
#include "thread.hpp"
#include "timer.hpp"



template <typename PIN, uint16_t PERIOD>
void *blinker(void *) {
    uint32_t last_tick = tick_now();

    for (;;) {          
        if (tick_now() - last_tick >= PERIOD) {
            PIN::toggle();
            last_tick = tick_now();
        }
        yield(&tcb_main);
    }
    return nullptr;
}


static Thread<64> pin_red  ( blinker<DP13, 750>, nullptr );
static Thread<64> pin_blue ( blinker<DP12, 500>, nullptr );
static Thread<64> pin_green( blinker<DP11, 250>, nullptr );



int main(void) {
    DP13::output();
    DP12::output();
    DP11::output();

    uart_init();
    tick_init();
    sei();

    
    // simulate round robin, but I am about to define a real scheduler
    bool printed = false;
    for(;;) {
        yield(&pin_red.tcb  );
        yield(&pin_blue.tcb );
        yield(&pin_green.tcb);

        // after 5 seconds every thread has run all: print the bytes used
        if (tick_now() >= 5000 && printed == false) {
            printed = true;
            uart_print_u16(pin_red.stack_used()  );
            uart_print_u16(pin_blue.stack_used() );
            uart_print_u16(pin_green.stack_used());
        }
    }

}