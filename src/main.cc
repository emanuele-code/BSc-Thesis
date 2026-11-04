#include "utilityPin.h"

#include <util/delay.h>



int main(void) {
    P13::output();
    P12::output();

    P13::high();
    for(;;) {
        P13::toggle();
        P12::toggle();
        _delay_ms(500);
    }
}
