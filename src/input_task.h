#pragma once

#include "FreeRTOS.h"

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

void input_task_init(void);

// This task is responsible for generating the input report.
void input_task_start(UBaseType_t priority, TickType_t interval);
