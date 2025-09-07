#pragma once

#include <stdbool.h>
#include <stdint.h>

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

typedef struct keyboard_output {
    float forward_duty_cycle;     // Bound between 0.0 and 1.0
    float left_duty_cycle;        // Bound between 0.0 and 1.0
    float right_duty_cycle;       // Bound between 0.0 and 1.0
    float reverse_duty_cycle;     // Bound between 0.0 and 1.0
    float hand_brake_duty_cycle;  // Bound between 0.0 and 1.0
} keyboard_output_t;

typedef struct input_output_map_config {
    // =========================================================================
    // Deadzones
    // =========================================================================

    // Minimum application of the pedal before input is applied
    float pedal_deadzone;

    // Minimum application of the tillers before input is applied
    float tiller_deadzone;

    // =========================================================================
    // Tiler behaviour
    // =========================================================================

    // How far back the tiller must be pulled to apply 100% turning
    float tiller_max_turn_threshold;

    // How far back the tiller must be pulled to engage the handbrake.
    // The handbrake application will be scalled from 0.0 at begin and will ramp
    // up to 1.0 at end. Pulling the tiller beyond end will keep the handbrake
    // engaged.
    float tiller_handbrake_threshold_begin;
    float tiller_handbrake_threshold_end;

} input_output_map_config_t;

const char* input_gear_to_str(input_gear_t gear);

void input_report_print(const input_report_t* report);

keyboard_output_t map_input_to_output(const input_output_map_config_t* config, const input_report_t* input);
