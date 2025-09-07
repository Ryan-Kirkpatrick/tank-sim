#pragma once

#include "FreeRTOS.h"

void input_task_init(void);

// This task is responsible for generating the input report.
void input_task_start(UBaseType_t priority, TickType_t interval);
