#include "dmx.h"
#include "hardware/sync.h"
#include "hardware/uart.h"

void dmx_init(dmx_driver_t* driver, uart_inst_t* uart, uint baud, uint pin) {
    driver->uart = uart;
    driver->baud = baud;

    for(int i = 0; i < 512; i++) {
        driver->UNIVERSE[i] = 0;
    }

    uart_init(uart, baud);
    if(uart == uart0) {
        gpio_set_function(pin, UART_FUNCSEL_NUM(uart, UART0_TX_PIN));
    } else {
        gpio_set_function(pin, UART_FUNCSEL_NUM(uart, UART1_TX_PIN));
    }
    uart_set_hw_flow(uart, false, false);
    // DMX calls for 8 data bits, 2 stop bits, and no parity (see ANSI E1.11-2008)
    uart_set_format(uart, 8, 2, UART_PARITY_NONE);
}

void dmx_transmit_frame(dmx_driver_t* driver) {
    // Disable interrupts, this is time sensitive (may return and make this interrupt or DMA based to avoid holding up the core for so long)
    uint32_t saved_interrupts = save_and_disable_interrupts();
    
    // BREAK (Found this breaking method for DMX online)
    uart_set_baudrate(driver->uart, 45454);  // ~88us break
    uart_putc_raw(driver->uart, 0x00);
    
    uart_set_baudrate(driver->uart, driver->baud);


    restore_interrupts(saved_interrupts); // Sleep requires interrupts, also allows other things to possibly happen while we sleep
    sleep_us(12); // DMX requires a ~12us minimum hold time before the actual data frame starts
    saved_interrupts = save_and_disable_interrupts(); // Turn them back off

    // Flash the PICO's LED to show we are transmitting a frame
    gpio_put(25, !gpio_get(25)); 
    // DMX requires one channel (byte) of all zeros be sent before the actual frame
    uart_putc_raw(driver->uart, 0x00);
    uart_write_blocking(driver->uart, driver->UNIVERSE, 512); // Write all the data out
    restore_interrupts(saved_interrupts);
}

void dmx_set_channel(dmx_driver_t* driver, uint16_t channel, uint8_t value) {
    if (channel < 1 || channel > 512) {
        return;  // Channel out of range
    }

    // Set channel value (adjust for 0-based indexing)
    driver->UNIVERSE[channel - 1] = value;
}