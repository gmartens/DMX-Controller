#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/uart.h"
#include "hardware/sync.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

#include "buttons.h"

#define UART0_BAUD_RATE 9600
#define UART0_TX_PIN 12
#define UART0_RX_PIN 13

#define UART1_BAUD_RATE 250000
#define UART1_TX_PIN 20
#define UART1_RX_PIN 21

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

uint8_t channels[512] = {0};
void dmx_transmit_frame() {
    // Disable interrupts during transmission
    uint32_t saved_interrupts = save_and_disable_interrupts();
    
    // Send BREAK signal
    uart_set_baudrate(uart1, 45454);  // ~88us break
    uart_putc_raw(uart1, 0x00);
    
    // Reset to DMX standard baud rate
    uart_set_baudrate(uart1, UART1_BAUD_RATE);

    restore_interrupts(saved_interrupts);
    sleep_us(12);
    saved_interrupts = save_and_disable_interrupts();
    gpio_put(25, !gpio_get(25)); 
    // Send start code
    uart_putc_raw(uart1, 0x00);

    // Send channel data
    uart_write_blocking(uart1, channels, 512);
    restore_interrupts(saved_interrupts);
}

void dmx_set_channel(uint16_t channel, uint8_t value) {
    // Validate channel and value
    if (channel < 1 || channel > 512) {
        return;  // Channel out of range
    }
    
    // Set channel value (adjust for 0-based indexing)
    channels[channel - 1] = value;
}

volatile uint8_t UART_RX_WP = 0;
volatile char UART_RX[256] = {0};

void on_uart_rx() {
    while(uart_is_readable(uart0)) {
        UART_RX[UART_RX_WP] = uart_getc(uart0);
        UART_RX_WP++;
        if(UART_RX[UART_RX_WP - 1] == '\n') {
            UART_RX[UART_RX_WP] = '\0';
            UART_RX_WP = 0;
            break;
        }
    }
    //printf(UART_RX);
}

typedef struct {
    uint pwm_forward_slice;
    uint pwm_forward_channel;
    uint pwm_reverse_slice;
    uint pwm_reverse_channel;
    uint adc_channel;
    uint max_pos;
    uint min_pos;
    uint tolerance;
    float gain;
} FaderController;

void fader_controller_init(FaderController* controller, uint forward_pin, uint reverse_pin, uint adc_pin, uint max_pos,
                           uint min_pos, uint tolerance, float gain) {
    controller->max_pos = max_pos;
    controller->min_pos = min_pos;
    controller->tolerance = tolerance;
    controller->gain = gain;

    // Configure PWM pins
    gpio_set_function(forward_pin, GPIO_FUNC_PWM);
    gpio_set_function(reverse_pin, GPIO_FUNC_PWM);

    // Get PWM slices and channels
    controller->pwm_forward_slice = pwm_gpio_to_slice_num(forward_pin);
    controller->pwm_forward_channel = pwm_gpio_to_channel(forward_pin);
    controller->pwm_reverse_slice = pwm_gpio_to_slice_num(reverse_pin);
    controller->pwm_reverse_channel = pwm_gpio_to_channel(reverse_pin);

    // Configure PWM
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.0f); // woah floating point clock divider... mind blown
    pwm_config_set_wrap(&config, 4095);
    
    pwm_init(controller->pwm_forward_slice, &config, true);
    pwm_init(controller->pwm_reverse_slice, &config, true);

    // Configure ADC
    adc_init();
    adc_gpio_init(adc_pin);
    controller->adc_channel = adc_pin == 26 ? 0 : adc_pin == 27 ? 1 : 2; // Hacky way to get the ADC channel from the GPIO pin number
}

void stop_motor(FaderController* controller) {
    pwm_set_chan_level(controller->pwm_forward_slice, controller->pwm_forward_channel, 0);
    pwm_set_chan_level(controller->pwm_reverse_slice, controller->pwm_reverse_channel, 0);
}

void move_motor(FaderController* controller, int32_t speed) {
    speed = speed > 4095 ? 4095 : speed;
    speed = speed < -4096 ? -4096 : speed;
    speed = speed > 0 ? 2458 + (speed/2.5) : -2458 + (speed/2.5);

    if (speed > 0) {
        pwm_set_chan_level(controller->pwm_forward_slice, controller->pwm_forward_channel, speed);
        pwm_set_chan_level(controller->pwm_reverse_slice, controller->pwm_reverse_channel, 0);
    } else if (speed < 0) {
        pwm_set_chan_level(controller->pwm_forward_slice, controller->pwm_forward_channel, 0);
        pwm_set_chan_level(controller->pwm_reverse_slice, controller->pwm_reverse_channel, -speed);
    } else {
        stop_motor(controller);
    }
}

uint16_t fader_get_current_position(FaderController* controller) {
    adc_select_input(controller->adc_channel);
    return adc_read();
}

bool fader_move_to_position(FaderController* controller, uint16_t target_position) {
        // Constrain target position
    target_position = (target_position > controller->max_pos) ? controller->max_pos : 
                      (target_position < controller->min_pos) ? controller->min_pos : 
                      target_position;

    int attempts = 0;
    const int max_attempts = 200;

    while (attempts < max_attempts) {
        uint16_t current_position = fader_get_current_position(controller);
        int16_t error = target_position - current_position;

        // Check if within tolerance
        if ((error < 0 ? -error : error) <= controller->tolerance) {
            stop_motor(controller);
            return true;
        }

        // Proportional control
        int16_t speed = (int16_t)(controller->gain * error);
        move_motor(controller, speed);

        sleep_ms(10);  // Small delay to allow motor movement
        attempts++;
    }

    // Failed to reach position
    stop_motor(controller);
    return false;
}

int main()
{
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    stdio_usb_init();

    // uart_init(uart0, UART0_BAUD_RATE);
    // gpio_set_function(UART0_TX_PIN, UART_FUNCSEL_NUM(uart0, UART0_TX_PIN));
    // gpio_set_function(UART0_RX_PIN, UART_FUNCSEL_NUM(uart0, UART0_RX_PIN));
    // uart_set_hw_flow(uart0, false, false);
    // irq_set_exclusive_handler(UART0_IRQ, on_uart_rx);
    // irq_set_enabled(UART0_IRQ, true);
    // uart_set_irq_enables(uart0, true, false);

    uart_init(uart1, UART1_BAUD_RATE);
    gpio_set_function(UART1_TX_PIN, UART_FUNCSEL_NUM(uart1, UART1_TX_PIN));
    gpio_set_function(UART1_RX_PIN, UART_FUNCSEL_NUM(uart1, UART1_RX_PIN));
    uart_set_hw_flow(uart1, false, false);
    uart_set_format(uart1, 8, 2, UART_PARITY_NONE);

    FaderController fader1;
    fader_controller_init(&fader1, 0, 2, 26, 4095, 21, 10, 0.4);
    FaderController fader2;
    fader_controller_init(&fader2, 6, 4, 27, 4095, 21, 10, 0.4);
    FaderController fader3;
    fader_controller_init(&fader3, 10, 8, 28, 4095, 21, 10, 0.4);
    
    init_buttons();

    repeating_timer_t button_scan_repeating_timer;
    add_repeating_timer_ms(1, button_scan, NULL, &button_scan_repeating_timer);

    gpio_put(25, 1);

    while (true) {
        // dmx_set_channel(1, B2_bm&buttons ? 255 : 0);
        // dmx_set_channel(2, B6_bm&buttons ? 255 : 0);
        // dmx_set_channel(3, B10_bm&buttons ? 255 : 0);

        // dmx_set_channel(7, B13_bm&buttons ? 0 : 255);

        // dmx_transmit_frame();
        fader_move_to_position(&fader1, 200);
        fader_move_to_position(&fader2, 200);
        fader_move_to_position(&fader3, 200);
        sleep_ms(1000);
        fader_move_to_position(&fader1, 4000);
        fader_move_to_position(&fader2, 4000);
        fader_move_to_position(&fader3, 4000);
        sleep_ms(1000);
        // printf("fader1: %d\n", fader_get_current_position(&fader1));
        // printf("fader2: %d\n", fader_get_current_position(&fader2));
        // printf("fader3: %d\n", fader_get_current_position(&fader3));
        // sleep_ms(500);
    }
}
