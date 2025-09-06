#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "tank_assert.h"
#include "task.h"

#define STACK_OVERFLOW_MSG "Stack overflow in "

void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName) {
    char message_buffer[128] = {0};
    snprintf(message_buffer, sizeof(message_buffer), "Stack overflow in %s", pcTaskName);
    TANK_ASSERT_M(false, message_buffer);
}