#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "portmacro.h"

typedef struct keyboard_output {
    float forward_duty_cycle;  // Bound between 0.0 and 1.0
    float left_duty_cycle;     // Bound between 0.0 and 1.0
    float right_duty_cycle;    // Bound between 0.0 and 1.0
    float reverse_duty_cycle;  // Bound between 0.0 and 1.0
    bool hand_brake;
} keyboard_output_t;

// Init the keyboard task
void keyboard_task_init(TickType_t pwm_period);

// This task is responsible for sending keyboard reports.
// Keyboard reports describe how "keyboard" buttons wil be pressed.
void keyboard_task_start(UBaseType_t priority, TickType_t interval);

// Sets the keyboard output that will be sent.
void keyboard_task_set_output(const keyboard_output_t* command);