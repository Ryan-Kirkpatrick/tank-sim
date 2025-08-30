#include "FreeRTOS.h"
#include "hardware/exception.h"
#include "task.h"

#include <pico/time.h>
#include "pico/stdlib.h"

#define STACK_SIZE 512

#define LED_PIN 25

StackType_t stack[STACK_SIZE];
StaticTask_t task_handle;

void task(void* unused) {
    while (1) {
        gpio_put(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_put(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void) {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    xTaskCreateStatic(task, "", STACK_SIZE, NULL, 1, stack, &task_handle);
    vTaskStartScheduler();

    // while (1) {
    //     gpio_put(LED_PIN, 1);
    //     sleep_ms(1000);
    //     gpio_put(LED_PIN, 0);
    //     sleep_ms(1000);
    // }
}
