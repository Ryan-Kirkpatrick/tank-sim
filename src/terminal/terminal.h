#pragma once

#include "FreeRTOS.h"
#include "portmacro.h"

typedef enum log_level {
    LOG_DEBUG = 0,
    LOG_INFO = 10,
    LOG_WARN = 20,
    LOG_CRITICAL = 30,
    LOG_NONE = 40,
    LOG_ALWAYS = 50,  // This level will always log, even if logging is disabled. Intended for terminal output.
} log_level_t;

void terminal_log(log_level_t log_level, const char* tag, const char* message, ...);

void terminal_set_log_level(log_level_t log_level);

void terminal_task_init(void);

void terminal_task_start(UBaseType_t priority, TickType_t interval);

#define LOG_D(tag, msg, ...) terminal_log(LOG_DEBUG, tag, msg, ##__VA_ARGS__)
#define LOG_I(tag, msg, ...) terminal_log(LOG_INFO, tag, msg, ##__VA_ARGS__)
#define LOG_W(tag, msg, ...) terminal_log(LOG_WARN, tag, msg, ##__VA_ARGS__)
#define LOG_C(tag, msg, ...) terminal_log(LOG_CRITICAL, tag, msg, ##__VA_ARGS__)
#define LOG_A(tag, msg, ...) terminal_log(LOG_ALWAYS, tag, msg, ##__VA_ARGS__)