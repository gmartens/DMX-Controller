#include "pico/stdlib.h"

typedef struct {
    uint8_t UNIVERSE[512];
    uart_inst_t* uart;
    uint baud;
} dmx_driver_t;

void dmx_init(dmx_driver_t* driver, uart_inst_t* uart, uint baud, uint pin);
void dmx_transmit_frame(dmx_driver_t* driver);
void dmx_set_channel(dmx_driver_t* driver, uint16_t channel, uint8_t value);