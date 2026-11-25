/* utilityPin.hpp
 *
 * DESCRIPTION
 * Zero-cost abstraction for Arduino UNO digital pins 8..13 (port B).
 *
 * WHAT IT DOES
 * Simplifies pin handling on the board by hiding the direct manipulation of
 * the DDRB/PORTB registers behind a typed interface, e.g. DP13::toggle().
 * The pin number is a template parameter, so everything is resolved at
 * compile time and generates the same code as writing the registers by hand.
 *
 * LIMITS
 * Only pins 8..13 (PB0..PB5) are supported.
 */
 


#pragma once

#include <avr/io.h>
#include <stdint.h>





/* the template is used to determine the pin number at compile time  */
template <uint8_t N>
struct UtilityPin {
    /* static_assert is evaluated at compile time: if the template argument N does not
     * satisfy the condition, compilation fails and no .elf is generated. */
    static_assert( N >= 2 && N <= 13, "Only pins 2..13 (ports D and B) are supported");

    /* like #define, but evaluated by the compiler (not the preprocessor):
     * it has a type and a scope, and no RAM is used.
     * Port D: the bit equals the pin number. Port B: pin 8 is bit 0. */
    static constexpr uint8_t BIT = (N < 8) ? N : (N - 8);

    /* static member functions: to avoid obj instance and save on RAM.
     * if constexpr keeps only the branch of the right port: no runtime cost. */
    static void output() {
        if constexpr (N < 8) DDRD |=  (1 << BIT);
        else                 DDRB |=  (1 << BIT);
    }
    static void input() {
        if constexpr (N < 8) DDRD &= ~(1 << BIT);
        else                 DDRB &= ~(1 << BIT);
    }
    static void high() {
        if constexpr (N < 8) PORTD |=  (1 << BIT);
        else                 PORTB |=  (1 << BIT);
    }
    static void low() {
        if constexpr (N < 8) PORTD &= ~(1 << BIT);
        else                 PORTB &= ~(1 << BIT);
    }
    static void toggle() {
        if constexpr (N < 8) PORTD ^= (1 << BIT);
        else                 PORTB ^= (1 << BIT);
    }
    static bool read() {
        if constexpr (N < 8) return PIND & (1 << BIT);
        else                 return PINB & (1 << BIT);
    }
};





using DP13 = UtilityPin<13>;
using DP12 = UtilityPin<12>;
using DP11 = UtilityPin<11>;
using DP10 = UtilityPin<10>;
using DP9  = UtilityPin<9>;
using DP8  = UtilityPin<8>;
using DP7  = UtilityPin<7>;
using DP6  = UtilityPin<6>;
using DP5  = UtilityPin<5>;
using DP4  = UtilityPin<4>;
using DP3  = UtilityPin<3>;
using DP2  = UtilityPin<2>;