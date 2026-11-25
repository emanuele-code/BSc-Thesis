/* utilityPin.hpp
 *
 * DESCRIPTION
 * Zero-cost abstraction for Arduino UNO digital pins 2..13 (port D: 2..7, port B: 8..13).
 *
 * WHAT IT DOES
 * Simplifies pin handling on the board by hiding the direct manipulation of
 * the DDRx/PORTx/PINx registers behind a typed interface, e.g. DP13::toggle().
 * The pin number is a template parameter, so everything is resolved at
 * compile time and generates the same code as writing the registers by hand.
 *
 * LIMITS
 * - Only pins 2..13 are supported: pins 0 and 1 are the UART (used by utilitySerial.hpp)
 *   and the analog pins A0..A5 are not supported
 * - read() returns true when the pin is high: a button wired to GND with the internal
 *   pull-up (input() then high()) reads false while it is pressed
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

    /* DDRx bit = 1: the pin drives the line */
    static void output() {
        if constexpr (N < 8) DDRD |=  (1 << BIT);
        else                 DDRB |=  (1 << BIT);
    }
    /* DDRx bit = 0: the pin only reads the line */
    static void input() {
        if constexpr (N < 8) DDRD &= ~(1 << BIT);
        else                 DDRB &= ~(1 << BIT);
    }
    /* output: drives the pin high. input: enables the internal pull-up */
    static void high() {
        if constexpr (N < 8) PORTD |=  (1 << BIT);
        else                 PORTB |=  (1 << BIT);
    }
    /* output: drives the pin low. input: disables the internal pull-up */
    static void low() {
        if constexpr (N < 8) PORTD &= ~(1 << BIT);
        else                 PORTB &= ~(1 << BIT);
    }
    /* output: inverts the pin level */
    static void toggle() {
        if constexpr (N < 8) PORTD ^= (1 << BIT);
        else                 PORTB ^= (1 << BIT);
    }
    /* reads the level of the pin from the PINx register: true = high */
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