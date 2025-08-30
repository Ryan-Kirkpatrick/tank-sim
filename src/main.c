#include "FreeRTOS.h"
#include "class/hid/hid_device.h"
#include "device/usbd.h"
#include "hardware/adc.h"
#include "hardware/exception.h"
#include "portmacro.h"
#include "projdefs.h"
#include "reporter.h"
#include "task.h"

#include <pico/time.h>
#include "pico/stdlib.h"
#include "tusb.h"

#include "pins.h"
#include "usb.h"

#define STACK_SIZE 1024 * 8

#define LED_PIN 25
#define POT_PIN 26

StackType_t led_task_stack[STACK_SIZE];
StaticTask_t led_task_handle;

StackType_t usb_task_stack[STACK_SIZE];
StaticTask_t usb_task_handle;

StackType_t pot_task_stack[STACK_SIZE];
StaticTask_t pot_task_handle;

void send_key(uint8_t keycode) {
    uint8_t report[8] = {0};  // modifiers + reserved + 6 keys
    report[2] = keycode;      // key press
    tud_hid_keyboard_report(0, 0, report);
}

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

void pot_task(void* unused) {
    adc_init();
    adc_gpio_init(POTENTIOMETER_PIN);
    adc_select_input(0);

    while (1) {
        uint16_t raw = adc_read();
        delay = max_delay - ((raw * max_delay) / 4095);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

int main(void) {
    usb_init();
    reporter_init(pdTICKS_TO_MS(100));

    reporter_report_t report = {.forward_duty_cycle = 0.9,
                                .left_duty_cycle = 0.50,
                                .right_duty_cycle = 0.25,
                                .reverse_duty_cycle = 0,
                                .hand_brake = false};

    reporter_set_report(&report);

    usb_start(1, 1);
    reporter_start(3, 15);

    xTaskCreateStatic(led_task, "", STACK_SIZE, NULL, 2, led_task_stack, &led_task_handle);
    xTaskCreateStatic(pot_task, "", STACK_SIZE, NULL, 2, pot_task_stack, &pot_task_handle);
    vTaskStartScheduler();

    // tusb_init();

    // int i = 0;

    // while (1) {
    //     // TinyUSB background tasks
    //     tud_task();

    //     // Check if device is ready to send HID reports
    //     if (tud_hid_ready()) {
    //         // HID keyboard report: modifiers=0, keys[6]={space}, rest zero
    //         uint8_t keycode[6] = {0x2C, 0, 0, 0, 0, 0};  // 0x2C = space

    //         if (i % 100 == 0) {
    //             keycode[0] = 0x00;
    //         }

    //         // Send keyboard report (interface 0)
    //         tud_hid_keyboard_report(0, 0, keycode);

    //         i++;
    //     }
    // }
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer,
                           uint16_t bufsize) {
    // we don’t need to handle this for a basic keyboard
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer,
                               uint16_t reqlen) {
    // we don’t need to provide anything for basic keyboard
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;  // return zero bytes
}

// uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
//     (void)instance;
//     static uint8_t const report[] = {TUD_HID_REPORT_DESC_KEYBOARD()};
//     return report;
// }
