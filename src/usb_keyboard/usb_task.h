#pragma once

#include "FreeRTOS.h"

// Init the usb task.
void usb_task_init(void);

// Start the keyboard task. This task is responsible for handling USB events.
// All it really does is tick tinyusb.
void usb_task_start(UBaseType_t priority, TickType_t interval);

