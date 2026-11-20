/* userSetup.cc
 *
 * DESCRIPTION
 * Example of user code: three LEDs that blink at different periods.
 *
 * WHAT IT DOES
 * setup() sets the pins 13, 12 and 11 as output and creates one thread per LED.
 * All the threads run the same function, blinker, with the pin and the period given
 * as template parameters, so they are resolved at compile time. The threads start
 * running when main calls thread_yield for the first time.
 *
 * LIMITS
 * - The blinker waits by polling the tick, so it uses the CPU also when it has
 *   nothing to do
 */



#include <stdint.h>

#include "utilityPin.hpp"
#include "thread.hpp"
#include "timer.hpp"




template <typename PIN, uint16_t PERIOD>
void *blinker(void *args) {
    uint32_t last_tick = tick_now();

    for (;;) {
        if (tick_now() - last_tick >= PERIOD) {
            PIN::toggle();
            last_tick += PERIOD;
        }
        thread_yield();                
    }
    return nullptr;                    
}



/* static: TCB and stack must live for the whole program, one Thread per LED */
static Thread<64> pin_red;
static Thread<64> pin_blue;
static Thread<64> pin_green;



/* Called once by main: configure the pins and create the threads.
   The threads are only registered in the scheduler here, they start later. */
void setup() {
    DP13::output();
    DP12::output();
    DP11::output();

    thread_create(&pin_red  , blinker<DP13, 1000>, &pin_red  );
    thread_create(&pin_green, blinker<DP11, 250 >, &pin_green);
    thread_create(&pin_blue , blinker<DP12, 500 >, &pin_blue );
}