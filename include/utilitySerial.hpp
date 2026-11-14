/* utilitySerial.hpp
 *
 * IMPORTANT
 * It has no real function in the library besides helping me with debugging or estimates bytes usage
 *
 * DESCRIPTION
 * Minimal module for the USART0 of the ATmega328P (Arduino UNO serial port).
 *
 * WHAT IT DOES
 * Configures the USART0 as 9600 baud and sends data one byte at a time, 
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
   of the datasheet: the USART divides the CPU clock by 16 * (UBRR + 1) to get the baud
   rate. It is 103 at 16 MHz and 9600 baud */
constexpr uint16_t UART_UBRR = F_CPU / 16 / UART_BAUD - 1;




/* Configure the USART0 as 9600 baud, 8 data bits, no parity, 1 stop bit (8N1), TX only.
 * UBRR0H/UBRR0L form the 12-bit baud rate register: UART_UBRR is split in its high
 * and low byte. TXEN0 (in UCSR0B) enables the transmitter. UCSZ01 and UCSZ00 (in UCSR0C)
 * select 8 data bits; parity off and 1 stop bit are already the reset defaults. */
inline void uart_init() {
    UBRR0H = UART_UBRR >> 8;
    UBRR0L = UART_UBRR & 0xFF;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}




/* Send one byte over the serial port (blocking).
 * UCSR0A is the USART status register: its bit UDRE0 (Data Register Empty) is 1 when
 * the transmit buffer is free and can accept a new byte. The while loop repeats while
 * that bit is 0, so its empty body (;) just waits (busy waiting).
 * Writing the byte in UDR0 (USART data register) hands it to the hardware, which sends
 * it one bit at a time without involving the CPU. */
inline void uart_put(char c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}




/* Print an unsigned 16-bit number in decimal followed by a new line.
 * The UART sends characters, not numbers, so each digit is converted to its ASCII
 * character ('0' + digit). The digits are extracted from the least significant one
 * (n % 10 is the last digit, n /= 10 drops it), so they are stored in buf in reverse
 * order and sent backwards. A do-while is used so that n = 0 still prints "0".
 * buf holds 5 characters because 65535 has at most 5 digits.
 * '\n' moves to the next line and '\r' returns to the start of the line. */
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

