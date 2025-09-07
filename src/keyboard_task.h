#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "portmacro.h"
#include "control_map.h"

// Init the keyboard task
void keyboard_task_init(TickType_t pwm_period);

// This task is responsible for sending keyboard reports.
// Keyboard reports describe how "keyboard" buttons wil be pressed.
void keyboard_task_start(UBaseType_t priority, TickType_t interval);

// Sets the keyboard output that will be sent.
void keyboard_task_set_output(const keyboard_output_t* command);