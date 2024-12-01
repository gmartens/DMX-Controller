/* 
 * File:   usart.h
 * Author: Gabe
 *
 * Created on November 30, 2024, 10:37 PM
 */

#ifndef USART_H
#define	USART_H

#ifdef	__cplusplus
extern "C" {
#endif

#define F_CPU 3333333

#define BUF_SIZE 128

#define SAMPLES_PER_BIT 16
#define USART_BAUD_VALUE(BAUD_RATE) (uint16_t) ((F_CPU << 6) / (((float) SAMPLES_PER_BIT) * (BAUD_RATE)) + 0.5)

void usart0Init(const uint32_t baud);

void usart0WriteChar(char c);

void usart0WriteCommand(const char *cmd);

char usart0ReadChar();

void usart0ReadUntil(char *dest, const char *end_str);

void usart1Init(const uint32_t baud);

void usart1WriteChar(char c);

void usart1WriteCommand(const char *cmd);

char usart1ReadChar();

void usart1ReadUntil(char *dest, const char *end_str);


#ifdef	__cplusplus
}
#endif

#endif	/* USART_H */

