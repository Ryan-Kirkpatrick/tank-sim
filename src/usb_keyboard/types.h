#pragma once

typedef struct keyboard_output {
    float forward_duty_cycle;     // Bound between 0.0 and 1.0
    float left_duty_cycle;        // Bound between 0.0 and 1.0
    float right_duty_cycle;       // Bound between 0.0 and 1.0
    float reverse_duty_cycle;     // Bound between 0.0 and 1.0
    float hand_brake_duty_cycle;  // Bound between 0.0 and 1.0
} keyboard_output_t;