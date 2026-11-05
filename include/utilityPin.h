/* utilityPin.h
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
    static_assert( N >= 8 && N <= 13, "Only pins 8..13 (port B) are supported");
    
    /* like #define, but evaluated by the compiler (not the preprocessor):
     * it has a type and a scope, and no RAM is used */
    static constexpr uint8_t BIT = N - 8; 
    

    /* static member functions: to avoid obj instance and save on RAM */
    static void output() { DDRB  |=  (1 << BIT); }
    static void input()  { DDRB  &= ~(1 << BIT); }
    static void high()   { PORTB |=  (1 << BIT); }
    static void low()    { PORTB &= ~(1 << BIT); }
    static void toggle() { PORTB ^=  (1 << BIT); }
};



using DP13 = UtilityPin<13>;
using DP12 = UtilityPin<12>;
using DP11 = UtilityPin<11>;
using DP10 = UtilityPin<10>;
using DP9  = UtilityPin<9>;
using DP8  = UtilityPin<8>;

