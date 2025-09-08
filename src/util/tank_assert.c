#include "util/tank_assert.h"

#include <stdarg.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

void _tank_assert(int assertion, const char* assertion_src, const char* file, const char* function, unsigned int line) {
    if (0 == assertion) {
        printf("ASSERTION FAILED:\r\n");
        printf("  Assertion: %s\r\n", assertion_src);
        printf("  Location: %s:%d\r\n", file, line);
        printf("  Function: %s\r\n", function);
        fflush(stdout);

        vTaskSuspendAll();
        while (1) {
            asm volatile("nop");
        }
    }
}

void _tank_assert_m(int assertion, const char* assertion_src, const char* file, const char* function, unsigned int line,
                    const char* fmt, ...) {
    if (0 == assertion) {
        va_list args;
        va_start(args, fmt);
        printf("ASSERTION FAILED:\r\n");
        printf("  Assertion: %s\r\n", assertion_src);
        printf("  Message: ");
        vprintf(fmt, args);
        printf("\r\n");
        printf("  Location: %s:%d\r\n", file, line);
        printf("  Function: %s\r\n", function);
        va_end(args);
        fflush(stdout);

        vTaskSuspendAll();
        while (1) {
            asm volatile("nop");
        }
    }
}
