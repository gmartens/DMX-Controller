#include <stdio.h>

// Pico SDK
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "pico/multicore.h"

// Project
#include "buttons.h"
#include "faders.h"
#include "dmx.h"

#define UART0_BAUD_RATE 9600
#define UART0_TX_PIN 12
#define UART0_RX_PIN 13

#define BUF_SIZE 256

#define ROW0 1
#define ROW1 3
#define ROW2 5
#define ROW3 7

#define COL0 22
#define COL1 19
#define COL2 18
#define COL3 15

volatile uint8_t FADER_VALUE[3] = {0};
volatile bool FADER_CHANGED[3] = {false, false, false};
uint8_t uart_rx_wp = 0;
char uart_rx[BUF_SIZE] = {0};
void on_uart_rx() {
    while(uart_is_readable(uart0)) {
        uart_rx[uart_rx_wp] = uart_getc(uart0);
        uart_rx_wp++;
        if(uart_rx[uart_rx_wp - 1] == '\n') {
            uart_rx[uart_rx_wp] = '\0';
            uart_rx_wp = 0;
            break;
        }
    }
    if(uart_rx_wp == 0) { // Make sure we only do this when we have read an entire message
        printf("CORE0> UART0 RX: %s\n", uart_rx);
        int fader_num = 0;
        int fader_value = 0;
        if(sscanf(uart_rx, "F%1d,%03d", &fader_num, &fader_value) == 2) {
            if(fader_num > 2 || fader_value > 255) {
                printf("CORE0> UART0 RX: Invalid values!\n");
                return;
            }
            FADER_VALUE[fader_num] = fader_value;
            FADER_CHANGED[fader_num] = true;
        }
    }
}

void core1_entry() {
    while(true) {
        fader_controller_t* fader = (fader_controller_t*)multicore_fifo_pop_blocking();
        uint32_t fader_position = multicore_fifo_pop_blocking();
        printf("CORE1> MOVING FADER ADC%d to %d\n", fader->adc_channel, fader_position);
        fader_set_pos(fader, fader_position);
    }
}

void core1_move_fader(fader_controller_t* fader, uint32_t fader_position) {
    // Dump what fader we want moved and where onto core1's FIFO, it will handle it
    multicore_fifo_push_blocking((uint32_t)fader);
    multicore_fifo_push_blocking(fader_position);
}

int main()
{
    stdio_usb_init(); // Allows for the Pico to transmit STDOUT over USB (revolutionary stuff compared to the AVR)
    gpio_init(25); // Init the Pico's LED
    gpio_set_dir(25, GPIO_OUT);

    // Initialize all 3 faders
    fader_controller_t faders[3];
    fader_controller_init(&faders[0], 10, 8, 28, 4095, 21, 10, 0.5);
    fader_controller_init(&faders[1], 6, 4, 27, 4095, 21, 10, 0.5);
    fader_controller_init(&faders[2], 0, 2, 26, 4095, 21, 10, 0.5);

    // UART to communicate with the AVR-BLE (via an IRQ)
    uart_init(uart0, UART0_BAUD_RATE);
    gpio_set_function(UART0_TX_PIN, UART_FUNCSEL_NUM(uart0, UART0_TX_PIN));
    gpio_set_function(UART0_RX_PIN, UART_FUNCSEL_NUM(uart0, UART0_RX_PIN));
    uart_set_hw_flow(uart0, false, false);
    irq_set_exclusive_handler(UART0_IRQ, on_uart_rx);
    irq_set_enabled(UART0_IRQ, true);
    uart_set_irqs_enabled(uart0, true, false);
    
    // Start scanning the buttons
    button_board_t button_board;
    repeating_timer_t button_board_timer;
    button_init(&button_board, &button_board_timer, ROW0, ROW1, ROW2, ROW3, COL0, COL1, COL2, COL3);

    // Initialize DMX
    dmx_driver_t dmx;
    dmx_init(&dmx, uart1, 250000, 20);

    // Launch the second core which will handle all the fader movements (to not bog down the DMX transmission)
    multicore_launch_core1(core1_entry);

    while (true) {
        dmx_set_channel(&dmx, 1, B2_bm&button_board.buttons ? 255 : ((fader_get_pos(&faders[0]) >> 4) & 0xFE)); // mask off the last bit to reduce the noise
        dmx_set_channel(&dmx, 2, B6_bm&button_board.buttons ? 255 : ((fader_get_pos(&faders[1]) >> 4) & 0xFE));
        dmx_set_channel(&dmx, 3, B10_bm&button_board.buttons ? 255 : ((fader_get_pos(&faders[2]) >> 4) & 0xFE));

        dmx_set_channel(&dmx, 7, B13_bm&button_board.buttons ? 0 : 255);

        if(FADER_CHANGED[0]) {
            FADER_CHANGED[0] = false;
            core1_move_fader(&faders[0], FADER_VALUE[0] << 4);
        }
        if(FADER_CHANGED[1]) {
            FADER_CHANGED[1] = false;
            core1_move_fader(&faders[1], FADER_VALUE[1] << 4);
        }
        if(FADER_CHANGED[2]) {
            FADER_CHANGED[2] = false;
            core1_move_fader(&faders[2], FADER_VALUE[2] << 4);
        }

        dmx_transmit_frame(&dmx);
        sleep_ms(20); // timing for DMX not crucial, about 20ms is fine
    }
}