#include "buttons.h"
#include <stdio.h>

bool _button_scan(repeating_timer_t* rt) {
    button_board_t* buttons = (button_board_t*)rt->user_data;
    // Scan through each row and read each column into that bitmasked section of the button variable
    switch(buttons->_step) {
        case 0:
            gpio_put(buttons->row_pins[0], 1);
            buttons->_step++;
            break;
        case 1:
            buttons->buttons &= 0b1111111111110000;
            buttons->buttons |= ((gpio_get(buttons->column_pins[3]) << 3) | (gpio_get(buttons->column_pins[2]) << 2) | (gpio_get(buttons->column_pins[1]) << 1) | (gpio_get(buttons->column_pins[0]) << 0)) << 0;
            gpio_put(buttons->row_pins[0], 0);
            buttons->_step++;
            break;
        case 2:
            gpio_put(buttons->row_pins[1], 1);
            buttons->_step++;
            break;
        case 3:
            buttons->buttons &= 0b1111111100001111;
            buttons->buttons |= ((gpio_get(buttons->column_pins[3]) << 3) | (gpio_get(buttons->column_pins[2]) << 2) | (gpio_get(buttons->column_pins[1]) << 1) | (gpio_get(buttons->column_pins[0]) << 0)) << 4;
            gpio_put(buttons->row_pins[1], 0);
            buttons->_step++;
            break;
        case 4:
            gpio_put(buttons->row_pins[2], 1);
            buttons->_step++;
            break;
        case 5:
            buttons->buttons &= 0b1111000011111111;
            buttons->buttons |= ((gpio_get(buttons->column_pins[3]) << 3) | (gpio_get(buttons->column_pins[2]) << 2) | (gpio_get(buttons->column_pins[1]) << 1) | (gpio_get(buttons->column_pins[0]) << 0)) << 8;
            gpio_put(buttons->row_pins[2], 0);
            buttons->_step++;
            break;
        case 6:
            gpio_put(buttons->row_pins[3], 1);
            buttons->_step++;
            break;
        case 7:
            buttons->buttons &= 0b0000111111111111;
            buttons->buttons |= ((gpio_get(buttons->column_pins[3]) << 3) | (gpio_get(buttons->column_pins[2]) << 2) | (gpio_get(buttons->column_pins[1]) << 1) | (gpio_get(buttons->column_pins[0]) << 0)) << 12;
            gpio_put(buttons->row_pins[3], 0);
            buttons->_step = 0;
            break;
        default:
            buttons->_step = 0;
            break;
    } 
    return true;
}

void button_init(button_board_t* buttons, repeating_timer_t* timer, uint8_t row0, uint8_t row1, uint8_t row2, uint8_t row3, uint8_t col0, uint8_t col1, uint8_t col2, uint8_t col3) {
    buttons->row_pins[0] = row0;
    buttons->row_pins[1] = row1;
    buttons->row_pins[2] = row2;
    buttons->row_pins[3] = row3;
    buttons->column_pins[0] = col0;
    buttons->column_pins[1] = col1;
    buttons->column_pins[2] = col2;
    buttons->column_pins[3] = col3;
    buttons->_step = 0;

    buttons->_timer = timer;

    // Set all rows to outputs
    gpio_init(buttons->row_pins[0]);
    gpio_set_dir(buttons->row_pins[0], GPIO_OUT);
    gpio_init(buttons->row_pins[1]);
    gpio_set_dir(buttons->row_pins[1], GPIO_OUT);
    gpio_init(buttons->row_pins[2]);
    gpio_set_dir(buttons->row_pins[2], GPIO_OUT);
    gpio_init(buttons->row_pins[3]);
    gpio_set_dir(buttons->row_pins[3], GPIO_OUT);

    // Set rows to a known value of zero
    gpio_put(buttons->row_pins[0], 0);
    gpio_put(buttons->row_pins[1], 0);
    gpio_put(buttons->row_pins[2], 0);
    gpio_put(buttons->row_pins[3], 0);

    // Set all columns to inputs with IPDs
    gpio_init(buttons->column_pins[0]);
    gpio_set_dir(buttons->column_pins[0], GPIO_IN);
    gpio_set_pulls(buttons->column_pins[0], false, true);
    gpio_init(buttons->column_pins[1]);
    gpio_set_dir(buttons->column_pins[1], GPIO_IN);
    gpio_set_pulls(buttons->column_pins[1], false, true);
    gpio_init(buttons->column_pins[2]);
    gpio_set_dir(buttons->column_pins[2], GPIO_IN);
    gpio_set_pulls(buttons->column_pins[2], false, true);
    gpio_init(buttons->column_pins[3]);
    gpio_set_dir(buttons->column_pins[3], GPIO_IN);
    gpio_set_pulls(buttons->column_pins[3], false, true);

    // Scan every ms
    add_repeating_timer_ms(1, _button_scan, buttons, buttons->_timer);
}