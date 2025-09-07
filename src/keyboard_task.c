#include "keyboard_task.h"

#include <hardware/gpio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "class/hid/hid_device.h"
#include "pins.h"
#include "portmacro.h"
#include "projdefs.h"
#include "queue.h"
#include "scan_codes.h"
#include "task.h"

// Task
#define KEYBOARD_TASK_STACK_SIZE (1024) / sizeof(StackType_t)
static StackType_t reporter_task_stack[KEYBOARD_TASK_STACK_SIZE];
static StaticTask_t reporter_task_control_block;
TickType_t keyboard_interval = 0;

// Queue
static QueueHandle_t keyboard_queue_handle;
static StaticQueue_t keyboard_queue_control_block;
static uint8_t queue_storage[sizeof(keyboard_output_t)];

// Report
static keyboard_output_t current_keyboard_output = {.forward_duty_cycle = 0.0,
                                                    .left_duty_cycle = 0.0,
                                                    .right_duty_cycle = 0.0,
                                                    .reverse_duty_cycle = 0.0,
                                                    .hand_brake_duty_cycle = 0.0};

// PWM
TickType_t reporter_pwm_period;
const uint32_t reporter_max_reports_before_reset_attempt = 1000;

void keyboard_task_init(TickType_t pwm_period) {
    // Set up PWM
    reporter_pwm_period = pwm_period;

    // Set up queue
    keyboard_queue_handle =
        xQueueCreateStatic(1, sizeof(keyboard_output_t), queue_storage, &keyboard_queue_control_block);

    // Set up engine switch
    gpio_init(ENGINE_ON_OFF_SWITCH_PIN);
    gpio_set_dir(ENGINE_ON_OFF_SWITCH_PIN, false);
    gpio_pull_up(ENGINE_ON_OFF_SWITCH_PIN);
}

void keyboard_task_set_output(const keyboard_output_t* command) {
    xQueueOverwrite(keyboard_queue_handle, command);
}

static void keyboard_send_report() {}

static void keyboard_task(void* unused) {
    vTaskDelay(1000);

    TickType_t wake_time = xTaskGetTickCount();
    TickType_t period_start = wake_time;

    while (1) {
        uint32_t n_reports_sent = 0;
        if (tud_hid_ready()) {
            // Determine how far we are through the period and start new periods
            const TickType_t current_tick = xTaskGetTickCount();
            TickType_t elapsed = current_tick - period_start;
            if (elapsed > reporter_pwm_period) {
                period_start = current_tick;
                elapsed = 0;
                keyboard_output_t new_report;
                if (pdPASS == xQueueReceive(keyboard_queue_handle, &new_report, 0)) {
                    current_keyboard_output = new_report;
                }
            }
            const float elapsed_fraction = (float)elapsed / (float)reporter_pwm_period;

            // Create HID report
            uint8_t key_codes[6] = {0x00};
            uint8_t key_codes_added = 0;

            if (elapsed_fraction < current_keyboard_output.forward_duty_cycle) {
                key_codes[key_codes_added] = SCAN_CODE_W;
                key_codes_added++;
            }
            if (elapsed_fraction < current_keyboard_output.left_duty_cycle) {
                key_codes[key_codes_added] = SCAN_CODE_A;
                key_codes_added++;
            }
            if (elapsed_fraction < current_keyboard_output.right_duty_cycle) {
                key_codes[key_codes_added] = SCAN_CODE_D;
                key_codes_added++;
            }
            if (elapsed_fraction < current_keyboard_output.reverse_duty_cycle) {
                key_codes[key_codes_added] = SCAN_CODE_S;
                key_codes_added++;
            }
            if (elapsed_fraction < current_keyboard_output.hand_brake_duty_cycle) {
                key_codes[key_codes_added] = SCAN_CODE_SPACEBAR;
                key_codes_added++;
            }

            // Clear report if the engine is disabled
            if (!gpio_get(ENGINE_ON_OFF_SWITCH_PIN)) {
                memset(key_codes, 0, 6);
                key_codes_added = 0;
            }

            // Clear the report if it time to send a reset
            if (reporter_max_reports_before_reset_attempt <= n_reports_sent) {
                // Every N reports send an empty one to unfuck stuck keys
                uint8_t zeros[8] = {0};
                n_reports_sent = 0;
                memset(key_codes, 0, 6);
                key_codes_added = 0;
            }

            // TODO send any single shot keys

            // Send report
            tud_hid_keyboard_report(0, 0, key_codes);
            n_reports_sent++;
        }
        vTaskDelayUntil(&wake_time, keyboard_interval);
    }
}

// Starts the reporter task.
void keyboard_task_start(UBaseType_t priority, TickType_t interval) {
    keyboard_interval = interval;
    xTaskCreateStatic(keyboard_task, "Keyboard Task", KEYBOARD_TASK_STACK_SIZE, NULL, priority, reporter_task_stack,
                      &reporter_task_control_block);
}
