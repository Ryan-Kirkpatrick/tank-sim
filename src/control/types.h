#pragma once

#include <stdint.h>

// Raw input
#define INPUT_RAW_PEDAL_ABSOLUTE_MIN ((uint16_t)0)
#define INPUT_RAW_PEDAL_ABSOLUTE_MAX ((uint16_t)4095)

// TODO verify tiller min and max values
#define INPUT_RAW_TILLER_ABSOLUTE_MIN ((int32_t)-8388608)
#define INPUT_RAW_TILLER_ABSOLUTE_MAX ((int32_t)8388607)

typedef struct input_raw_report {
    int16_t accelerator;   // Bound between INPUT_RAW_PEDAL_ABSOLUTE_MIN / MAX
    int16_t brake;         // Bound between INPUT_RAW_PEDAL_ABSOLUTE_MIN / MAX
    int16_t clutch;        // Bound between INPUT_RAW_PEDAL_ABSOLUTE_MIN / MAX
    int32_t left_tiller;   // Bound between INPUT_RAW_TILLER_ABSOLUTE_MIN / MAX
    int32_t right_tiller;  // Bound between INPUT_RAW_TILLER_ABSOLUTE_MIN / MAX
} control_raw_report_t;

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

} control_settings_t;