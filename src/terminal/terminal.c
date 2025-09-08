#include "terminal.h"

#include <hardware/uart.h>
#include <stdarg.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "pins.h"
#include "portmacro.h"
#include "semphr.h"
#include "util/tank_assert.h"

// Task info
#define TERMINAL_TASK_STACK_SIZE 1024 / sizeof(StackType_t)
static StackType_t terminal_task_stack[TERMINAL_TASK_STACK_SIZE];
static StaticTask_t terminal_task_control_block;
static TickType_t terminal_interval = 0;

// Mutex
StaticSemaphore_t terminal_mutex;
SemaphoreHandle_t terminal_mutex_handle;

// Log level
static log_level_t terminal_current_log_level = LOG_DEBUG;

// Terminal input
#define TERMINAL_INPUT_SIZE 512
static char terminal_input[TERMINAL_INPUT_SIZE] = {0};

static const char* log_level_to_message(log_level_t level) {
    switch (level) {
        case LOG_DEBUG:
            return "[DEBUG]";
        case LOG_INFO:
            return "[INFO]";
        case LOG_WARN:
            return "[WARN]";
        case LOG_CRITICAL:
            return "[CRITICAL]";
        default:
            return "";
    }
}

void terminal_set_log_level(log_level_t log_level) {
    terminal_current_log_level = log_level;
}

void terminal_log(log_level_t log_level, const char* tag, const char* message, ...) {
    // Supress unwanted messages
    if (log_level < terminal_current_log_level) {
        return;
    }

    TANK_ASSERT(pdTRUE == xSemaphoreTake(terminal_mutex_handle, portMAX_DELAY));

    va_list args;
    va_start(args, message);
    printf("\r[%s]%s :: ", tag, log_level_to_message(log_level));
    vprintf(message, args);
    printf("\r\n > %s", terminal_input);
    fflush(stdout);
    va_end(args);

    TANK_ASSERT(pdTRUE == xSemaphoreGive(terminal_mutex_handle));
}

void terminal_process_command(void) {
    printf("Echoing input...\r\n");
    printf("%s\r\n", terminal_input);
}

void terminal_task(void* unused) {
    TickType_t wake_time = xTaskGetTickCount();

    uint32_t char_index = 0;

    while (1) {
        TANK_ASSERT(pdTRUE == xSemaphoreTake(terminal_mutex_handle, portMAX_DELAY));

        while (uart_is_readable(STDIO_UART_ID) && char_index < (TERMINAL_INPUT_SIZE - 1)) {
            uint8_t current_char = uart_getc(STDIO_UART_ID);
            if (current_char == '\r' || current_char == '\n') {
                terminal_process_command();
                char_index = 0;
                terminal_input[char_index] = '\0';
                continue;
            }
            terminal_input[char_index] = current_char;
            char_index++;
            terminal_input[char_index] = '\0';
        }
        printf("\r > %s", terminal_input);
        TANK_ASSERT(pdTRUE == xSemaphoreGive(terminal_mutex_handle));
        vTaskDelayUntil(&wake_time, terminal_interval);
    }
}

void terminal_task_init() {
    terminal_mutex_handle = xSemaphoreCreateMutexStatic(&terminal_mutex);
}

void terminal_task_start(UBaseType_t priority, TickType_t interval) {
    terminal_interval = interval;
    xTaskCreateStatic(terminal_task, "Terminal Task", TERMINAL_TASK_STACK_SIZE, NULL, priority, terminal_task_stack,
                      &terminal_task_control_block);
}
