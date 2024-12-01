#ifndef F_CPU
#define F_CPU 3333333
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include "usart.h"

#define BLE_RADIO_PROMPT "CMD> "

void bleInit(const char *name) {
    // INIT USART0
    usart0Init(9600);
    
    // Put BLE Radio in "Application Mode" by driving F3 high
    PORTF.DIRSET = PIN3_bm;
    PORTF.OUTSET = PIN3_bm;

    // Reset BLE Module - pull PD3 low, then back high after a delay
    PORTD.DIRSET = PIN3_bm | PIN2_bm;
    PORTD.OUTCLR = PIN3_bm;
    _delay_ms(10); // Leave reset signal pulled low
    PORTD.OUTSET = PIN3_bm;

    // The AVR-BLE hardware guide is wrong. Labels this as D3
    // Tell BLE module to expect data - set D2 low
    PORTD.OUTCLR = PIN2_bm;
    _delay_ms(200); // Give time for RN4870 to boot up

    char buf[BUF_SIZE];
    // Put RN4870 in Command Mode
    usart0WriteCommand("$$$");
    usart0ReadUntil(buf, BLE_RADIO_PROMPT);

    // Change BLE device name to specified value
    // There can be some lag between updating name here and
    // seeing it in the LightBlue phone interface
    strcpy(buf, "S-,");
    strcat(buf, name);
    strcat(buf, "\r\n");
    usart0WriteCommand(buf);
    usart0ReadUntil(buf, BLE_RADIO_PROMPT);


    //Send command to remove all previously declared BLE services
    usart0WriteCommand("PZ\r\n");
    usart0ReadUntil(buf, BLE_RADIO_PROMPT);

    // Add a new service. Feel free to use any ID you want from the
    // BLE assigned numbers document. Avoid the "generic" services.
    usart0WriteCommand("PS,1810\r\n");
    usart0ReadUntil(buf, BLE_RADIO_PROMPT);

    // Add a new characteristic to the service for your accelerometer
    // data. Pick any ID you want from the BLE assigned numbers document.
    // Avoid the "generic" characteristics.
    usart0WriteCommand("PC,2AE4,0A,04\r\n");
    usart0ReadUntil(buf, BLE_RADIO_PROMPT);

    // Set the characteristic's initial value to hex "00".
    usart0WriteCommand("LS\r\n");
    usart0ReadUntil(buf, BLE_RADIO_PROMPT);
    
    
    usart0WriteCommand("SHW,0072,00\r\n");
    usart0ReadUntil(buf, BLE_RADIO_PROMPT);
    
}

int main() {
    bleInit("DMX");
    usart1Init(9600);
    
    char buf[BUF_SIZE];
    while (1) {
        usart1ReadUntil(buf, "\n");
        usart0WriteCommand("SHW,0072,99\r\n");
        usart0ReadUntil(buf, BLE_RADIO_PROMPT);
        _delay_ms(1000);
    }
}
