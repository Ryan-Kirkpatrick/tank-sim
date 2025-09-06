#include <hardware/gpio.h>
#include <hardware/structs/io_bank0.h>
#include <hardware/uart.h>
#include <pico/stdio.h>
#include <pico/time.h>
#include <stdint.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "class/hid/hid_device.h"
#include "device/usbd.h"
#include "hardware/adc.h"
#include "hardware/exception.h"
#include "hx710c.h"
#include "input_task.h"
#include "keyboard_task.h"
#include "pico/stdlib.h"
#include "pins.h"
#include "portmacro.h"
#include "projdefs.h"
#include "task.h"
#include "tusb.h"
#include "usb_task.h"

#define STACK_SIZE 1024 * 8

StackType_t led_task_stack[STACK_SIZE];
StaticTask_t led_task_handle;


const uint32_t max_delay = 500;
volatile uint32_t delay = max_delay;

void led_task(void* unused) {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (1) {
        gpio_put(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(delay));
        gpio_put(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
}

int main(void) {
    // Init uart
    gpio_set_function(STDIO_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(STDIO_UART_RX_PIN, GPIO_FUNC_UART);

    uart_init(STDIO_UART_ID, STDIO_UART_BAUD_RATE);
    uart_set_format(STDIO_UART_ID, 8, 1, UART_PARITY_NONE);

    // Stdio
    stdio_init_all();

    usb_task_init();
    keyboard_task_init(pdMS_TO_TICKS(200));
    input_task_init();

    keyboard_output_t report = {.forward_duty_cycle = 0.9,
                                .left_duty_cycle = 0.50,
                                .right_duty_cycle = 0.25,
                                .reverse_duty_cycle = 0,
                                .hand_brake = false};

    keyboard_task_set_output(&report);

    

    usb_task_start(1, 1);
    keyboard_task_start(4, pdMS_TO_TICKS(15));
    // input_task_start(3, pdMS_TO_TICKS(500));

    xTaskCreateStatic(led_task, "", STACK_SIZE, NULL, 2, led_task_stack, &led_task_handle);
    vTaskStartScheduler();
}