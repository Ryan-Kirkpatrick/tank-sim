#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "portmacro.h"

typedef struct reporter_command {
    float forward_duty_cycle;
    float left_duty_cycle;
    float right_duty_cycle;
    float reverse_duty_cycle;
    bool hand_brake;
} reporter_report_t;

void reporter_init(TickType_t pwm_period);

// Starts the reporter task.
void reporter_start(UBaseType_t priority, TickType_t interval);

// Sets the report that the reporter will report
void reporter_set_report(const reporter_report_t* command);