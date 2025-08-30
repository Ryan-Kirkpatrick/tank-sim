#pragma once

#include "FreeRTOS.h"

void usb_init(void);
void usb_start(UBaseType_t priority, TickType_t interval);

