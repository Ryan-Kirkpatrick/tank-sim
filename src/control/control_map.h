#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "types.h"
#include "usb_keyboard/types.h"

typedef enum input_gear {
    INPUT_FORWARDS,
    INPUT_REVERSE,
} input_gear_t;

typedef struct input_report {
    float left_tiller;   // Bound between 0.0 and 1.0
    float right_tiller;  // Bound between 0.0 and 1.0
    float accelerator;   // Bound between 0.0 and 1.0
    input_gear_t gear;
} input_report_t;

const char* input_gear_to_str(input_gear_t gear);

void input_report_print(const input_report_t* report);

keyboard_output_t map_input_to_output(const control_settings_t* config, const input_report_t* input);
