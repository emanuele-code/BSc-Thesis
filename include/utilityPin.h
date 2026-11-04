#pragma once

#include <avr/io.h>
#include <stdint.h>



template <uint8_t N>
struct UtilityPin {
    static_assert( N >= 8 && N <= 13, "Only pins 8..13 (port B) are supported");
    static constexpr uint8_t BIT = N - 8;
    
    static inline void output() { DDRB  |=  (1 << BIT); }
    static inline void input()  { DDRB  &= ~(1 << BIT); }
    static inline void high()   { PORTB |=  (1 << BIT); }
    static inline void low()    { PORTB &= ~(1 << BIT); }
    static inline void toggle() { PORTB ^=  (1 << BIT); }
};


using P13 = UtilityPin<13>;
using P12 = UtilityPin<12>;
using P11 = UtilityPin<11>;
using P10 = UtilityPin<10>;
using P9  = UtilityPin<9>;
using P8  = UtilityPin<8>;

