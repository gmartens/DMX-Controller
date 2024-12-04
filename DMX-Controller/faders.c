#include "faders.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"

void _fader_stop(fader_controller_t* controller) {
    // DRV8837 datasheet says 0,0 means outputs are Z,Z (floating)
    pwm_set_chan_level(controller->pwm_forward_slice, controller->pwm_forward_channel, 0);
    pwm_set_chan_level(controller->pwm_reverse_slice, controller->pwm_reverse_channel, 0);
}

void _fader_set_speed(fader_controller_t* controller, int32_t speed) {
    // I noticed the faders wouldn't really move if there wasn't enough power, so I clamp the speed to a minimum. This still sometimes isn't enough to move the fader.
    speed = speed > 0 ? 2458 + (speed/2.5) : -2458 + (speed/2.5);

    // Clamp speeds to the PWM limit
    speed = speed > 4095 ? 4095 : speed;
    speed = speed < -4095 ? -4095 : speed; // Technically 2's compliment would dicate we can go to -4096 but the PWM limit is 4095

    if (speed > 0) {
        // Forwards
        pwm_set_chan_level(controller->pwm_forward_slice, controller->pwm_forward_channel, speed);
        pwm_set_chan_level(controller->pwm_reverse_slice, controller->pwm_reverse_channel, 0);
    } else if (speed < 0) {
        // Reverse
        pwm_set_chan_level(controller->pwm_forward_slice, controller->pwm_forward_channel, 0);
        pwm_set_chan_level(controller->pwm_reverse_slice, controller->pwm_reverse_channel, -speed);
    } else {
        // Zero (stop)
        _fader_stop(controller);
    }
}

void fader_controller_init(fader_controller_t* controller, uint forward_pin, uint reverse_pin, uint adc_pin, uint max_pos,
                           uint min_pos, uint tolerance, float gain) {
    controller->max_pos = max_pos;
    controller->min_pos = min_pos;
    controller->tolerance = tolerance;
    controller->gain = gain;

    gpio_set_function(forward_pin, GPIO_FUNC_PWM);
    gpio_set_function(reverse_pin, GPIO_FUNC_PWM);

    // Get PWM IDs since we need them to control the motor
    controller->pwm_forward_slice = pwm_gpio_to_slice_num(forward_pin);
    controller->pwm_forward_channel = pwm_gpio_to_channel(forward_pin);
    controller->pwm_reverse_slice = pwm_gpio_to_slice_num(reverse_pin);
    controller->pwm_reverse_channel = pwm_gpio_to_channel(reverse_pin);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.0f); // woah floating point clock divider... mind blown
    pwm_config_set_wrap(&config, 4095); 
    
    pwm_init(controller->pwm_forward_slice, &config, true);
    pwm_init(controller->pwm_reverse_slice, &config, true);

    adc_init();
    adc_gpio_init(adc_pin);
    controller->adc_channel = adc_pin == 26 ? 0 : adc_pin == 27 ? 1 : 2; // Hacky way to get the ADC channel from the GPIO pin number (see RP2040 datasheet)
}

uint16_t fader_get_pos(fader_controller_t* controller) {
    adc_select_input(controller->adc_channel);
    return adc_read();
}

bool fader_set_pos(fader_controller_t* controller, uint16_t target_position) {
    // Constrain target position, since the fader isn't quite perfect and the top and bottom aren't quite 0 -> 4095
    target_position = (target_position > controller->max_pos) ? controller->max_pos : 
                      (target_position < controller->min_pos) ? controller->min_pos : target_position;


    // The below algorithm is a gain based motion control algorithm. I stole the idea from online.
    // I haven't taken a control systems class yet :)
    // I also tried a PID algorithm, but it didn't seem to improve things but vastly increased complexity, may be worth investigating again
    int attempts = 0;
    while (attempts < MAX_TRIES) {
        uint16_t current_position = fader_get_pos(controller);
        int16_t error = target_position - current_position;

        // Check if within tolerance (abs function not included in stdlib)
        if ((error < 0 ? -error : error) <= controller->tolerance) {
            _fader_stop(controller);
            return true;
        }

        // Gain based control
        int16_t speed = (int16_t)(controller->gain * error);
        _fader_set_speed(controller, speed);

        sleep_ms(5);  // This is meant to run on another core, so delaying here is fine (wait for motor movement)
        attempts++;
    }

    // Failed to reach position
    _fader_stop(controller);
    return false;
}