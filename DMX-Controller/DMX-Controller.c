#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/uart.h"

#define UART0_BAUD_RATE 9600
#define UART0_TX_PIN 12
#define UART0_RX_PIN 13

#define ROW0 1
#define ROW1 3
#define ROW2 5
#define ROW3 7

#define COL0 22
#define COL1 19
#define COL2 18
#define COL3 15

volatile uint8_t step = 0;
volatile uint16_t buttons = 0;
bool button_scan(repeating_timer_t* rt) {
    switch(step) {
        case 0:
            gpio_put(ROW0, 1);
            step++;
            break;
        case 1:
            buttons &= 0b1111111111110000;
            buttons |= ((gpio_get(COL3) << 3) | (gpio_get(COL2) << 2) | (gpio_get(COL1) << 1) | (gpio_get(COL0) << 0)) << 0;
            gpio_put(ROW0, 0);
            step++;
            break;
        case 2:
            gpio_put(ROW1, 1);
            step++;
            break;
        case 3:
            buttons &= 0b1111111100001111;
            buttons |= ((gpio_get(COL3) << 3) | (gpio_get(COL2) << 2) | (gpio_get(COL1) << 1) | (gpio_get(COL0) << 0)) << 4;
            gpio_put(ROW1, 0);
            step++;
            break;
        case 4:
            gpio_put(ROW2, 1);
            step++;
            break;
        case 5:
            buttons &= 0b1111000011111111;
            buttons |= ((gpio_get(COL3) << 3) | (gpio_get(COL2) << 2) | (gpio_get(COL1) << 1) | (gpio_get(COL0) << 0)) << 8;
            gpio_put(ROW2, 0);
            step++;
            break;
        case 6:
            gpio_put(ROW3, 1);
            step++;
            break;
        case 7:
            buttons &= 0b0000111111111111;
            buttons |= ((gpio_get(COL3) << 3) | (gpio_get(COL2) << 2) | (gpio_get(COL1) << 1) | (gpio_get(COL0) << 0)) << 12;
            gpio_put(ROW3, 0);
            step = 0;
            break;
    } 
    return true;
}

void init_buttons() {
    gpio_init(ROW0);
    gpio_set_dir(ROW0, GPIO_OUT);
    gpio_init(ROW1);
    gpio_set_dir(ROW1, GPIO_OUT);
    gpio_init(ROW2);
    gpio_set_dir(ROW2, GPIO_OUT);
    gpio_init(ROW3);
    gpio_set_dir(ROW3, GPIO_OUT);

    gpio_put(ROW0, 0);
    gpio_put(ROW1, 0);
    gpio_put(ROW2, 0);
    gpio_put(ROW3, 0);

    gpio_init(COL0);
    gpio_set_dir(COL0, GPIO_IN);
    gpio_set_pulls(COL0, false, true);
    gpio_init(COL1);
    gpio_set_dir(COL1, GPIO_IN);
    gpio_set_pulls(COL1, false, true);
    gpio_init(COL2);
    gpio_set_dir(COL2, GPIO_IN);
    gpio_set_pulls(COL2, false, true);
    gpio_init(COL3);
    gpio_set_dir(COL3, GPIO_IN);
    gpio_set_pulls(COL3, false, true);
}

int main()
{
    stdio_usb_init();

    repeating_timer_t button_scan_repeating_timer;
    add_repeating_timer_ms(1, button_scan, NULL, &button_scan_repeating_timer);

    // Set up our UART
    uart_init(uart0, UART0_BAUD_RATE);
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART0_TX_PIN, UART_FUNCSEL_NUM(uart0, UART0_TX_PIN));
    gpio_set_function(UART0_RX_PIN, UART_FUNCSEL_NUM(uart0, UART0_RX_PIN));
    uart_set_hw_flow(uart0, false, false);
    
    init_buttons();

    char BUF[128];
    while (true) {
        printf("%d\n", buttons);
        sleep_ms(1000);

        uart_puts(uart0, "Hello, UART!\n");
    }
}
