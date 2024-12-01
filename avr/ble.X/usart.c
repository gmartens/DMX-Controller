#include <avr/io.h>
#include <string.h>
#include "usart.h"

void usart0Init(const uint32_t baud) {
    PORTA.DIR |= PIN0_bm;
    PORTA.DIR &= ~PIN1_bm;
    USART0.BAUD = (uint16_t)USART_BAUD_VALUE(baud);
    USART0.CTRLB |= USART_TXEN_bm | USART_RXEN_bm;
}

void usart0WriteChar(char c) {
    while (!(USART0.STATUS & USART_DREIF_bm));
    USART0.TXDATAL = c;
}

void usart0WriteCommand(const char *cmd) {
    for (uint8_t i = 0; cmd[i] != '\0'; i++) {
        usart0WriteChar(cmd[i]);
    }
}

char usart0ReadChar() {
    while(!(USART0.STATUS & USART_RXCIF_bm));
    return USART0.RXDATAL;
}

void usart0ReadUntil(char *dest, const char *end_str) {
    // Zero out dest memory so we always have null terminator at end
    memset(dest, 0, BUF_SIZE);
    uint8_t end_len = strlen(end_str);
    uint8_t bytes_read = 0;
    while (bytes_read < end_len || strcmp(dest + bytes_read - end_len, end_str) != 0) {
        dest[bytes_read] = usart0ReadChar();
        bytes_read++;
    }
}

void usart1Init(const uint32_t baud) {
    PORTC.DIR |= PIN0_bm;
    PORTC.DIR &= ~PIN1_bm;
    USART1.BAUD = (uint16_t)USART_BAUD_VALUE(baud);
    USART1.CTRLB |= USART_TXEN_bm | USART_RXEN_bm;
}

void usart1WriteChar(char c) {
    while (!(USART1.STATUS & USART_DREIF_bm));
    USART1.TXDATAL = c;
}

void usart1WriteCommand(const char *cmd) {
    for (uint8_t i = 0; cmd[i] != '\0'; i++) {
        usart1WriteChar(cmd[i]);
    }
}

char usart1ReadChar() {
    while(!(USART1.STATUS & USART_RXCIF_bm));
    return USART1.RXDATAL;
}

void usart1ReadUntil(char *dest, const char *end_str) {
    // Zero out dest memory so we always have null terminator at end
    memset(dest, 0, BUF_SIZE);
    uint8_t end_len = strlen(end_str);
    uint8_t bytes_read = 0;
    while (bytes_read < end_len || strcmp(dest + bytes_read - end_len, end_str) != 0) {
        dest[bytes_read] = usart1ReadChar();
        bytes_read++;
    }
}
