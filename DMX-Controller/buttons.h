#pragma once

#include "pico/stdlib.h"

#define B1_bm  0b0000000000000001 // 1
#define B5_bm  0b0000000000000010 // 2
#define B9_bm  0b0000000000000100 // 4
#define B13_bm 0b0000000000001000 // 8
#define B2_bm  0b0000000000010000 // 16
#define B6_bm  0b0000000000100000 // 32
#define B10_bm 0b0000000001000000 // 64
#define B14_bm 0b0000000010000000 // 128
#define B4_bm  0b0000000100000000 // 256
#define B8_bm  0b0000001000000000 // 512
#define B12_bm 0b0000010000000000 // 1024
#define B16_bm 0b0000100000000000 // 2048
#define B3_bm  0b0001000000000000 // 4096
#define B7_bm  0b0010000000000000 // 8192
#define B11_bm 0b0100000000000000 // 16384
#define B15_bm 0b1000000000000000 // 32768

typedef struct {
    volatile uint16_t buttons;
    uint8_t row_pins[4];
    uint8_t column_pins[4];
    uint8_t _step;
    repeating_timer_t* _timer;
} button_board_t;

void button_init(button_board_t* buttons, repeating_timer_t* timer, uint8_t row0, uint8_t row1, uint8_t row2, uint8_t row3, uint8_t col0, uint8_t col1, uint8_t col2, uint8_t col3);