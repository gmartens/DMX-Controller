#pragma once

#include "pico/stdlib.h"

#define MAX_TRIES 150

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
} fader_controller_t;

void fader_controller_init(fader_controller_t* controller, uint forward_pin, uint reverse_pin, uint adc_pin, uint max_pos,
                           uint min_pos, uint tolerance, float gain);

uint16_t fader_get_pos(fader_controller_t* controller);

bool fader_set_pos(fader_controller_t* controller, uint16_t target_position);