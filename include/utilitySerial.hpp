/* utilitySerial.hpp
 *
 * IMPORTANT
 * It has no real function in the library besides helping me with debugging or estimates bytes usage
 *
 * DESCRIPTION
 * Minimal module for the UART0 of the ATmega328P (Arduino UNO serial port).
 *
 * WHAT IT DOES
 * Configures the UART0 as 9600 baud and sends data one byte at a time, 
 * waiting for the transmit buffer to be free.
 * TX only: nothing is received.
 *
 * LIMITS
 * Blocking (busy waiting)
 */

 

#pragma once

#include <avr/io.h>
#include <stdint.h>



/* serial speed in bits per second: the serial monitor on the PC must use the same value */
constexpr uint32_t UART_BAUD = 9600;

/* value for the baud rate register UBRR0, computed at compile time with the formula
   of the datasheet: the UART divides the CPU clock by 16 * (UBRR + 1) to get the baud
   rate. It is 103 at 16 MHz and 9600 baud */
constexpr uint16_t UART_UBRR = F_CPU / (16*UART_BAUD) - 1;




/* Configure the UART0 as 9600 baud: UBRR0 = 103 makes the UART divide the CPU clock
   by 16 * (103 + 1) = 1664, so 16 000 000 / 1664 ~ 9600 bps */
inline void uart_init() {
    UBRR0H = UART_UBRR >> 8;
    UBRR0L = UART_UBRR & 0xFF;
    UCSR0B = (1 << TXEN0);                     // activate the transmission on TX pin
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);    // use 8N1 format, standard serial format
}




/* Send one byte over the serial port (blocking).
 * UCSR0A is the UART status register: its bit UDRE0 (Data Register Empty) is 1 when
 * the transmit buffer is free and can accept a new byte. The while loop repeats while
 * that bit is 0, so its empty body just waits (busy waiting).
 * Writing the byte in UDR0 (UART data register) the byte goes on transmission buffer and
 * the hardware sends to TX pin one bit at time */
inline void uart_put(char c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}




/* Print a number in decimal followed by a new line.
 * The UART sends characters, not numbers: each digit is converted to its character
 * ('0' + digit). The digits come out from the last one (n % 10), so they are stored
 * in buf and sent backwards. buf holds 5 digits because 65535 has at most 5. */
inline void uart_print_u16(uint16_t n) {
    char    buf[5];
    uint8_t len = 0;

    do {
        buf[len++] = '0' + n % 10;
        n /= 10;
    } while (n);

    while (len) uart_put(buf[--len]);
    uart_put('\n');
    uart_put('\r');
}

