#include "tank_assert.h"

#include <stdio.h>

void _tank_assert(int assertion, const char* assertion_src, const char* file, const char* function, unsigned int line) {
    if (0 == assertion) {
        printf("ASSERTION FAILED:\r\n");
        printf("  Assertion: %s\r\n", assertion_src);
        printf("  Location: %s:%d\r\n", file, line);
        printf("  Function: %s\r\n", function);
        while (1) {
            asm volatile("nop");
        }
    }
}

void _tank_assert_m(int assertion, const char* message, const char* assertion_src, const char* file,
                    const char* function, unsigned int line) {
    if (0 == assertion) {
        printf("ASSERTION FAILED:\r\n");
        printf("  Assertion: %s\r\n", assertion_src);
        printf("  Message: %s\r\n", message);
        printf("  Location: %s:%d\r\n", file, line);
        printf("  Function: %s\r\n", function);
        while (1) {
            asm volatile("nop");
        }
    }
}
