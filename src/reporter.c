#include "reporter.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "class/hid/hid_device.h"
#include "portmacro.h"
#include "projdefs.h"
#include "queue.h"
#include "scan_codes.h"
#include "task.h"

// Task
#define reporter_TASK_STACK_SIZE 1024 / sizeof(StackType_t)
static StackType_t reporter_task_stack[reporter_TASK_STACK_SIZE];
static StaticTask_t reporter_task_control_block;
TickType_t reporter_interval = 0;

// Queue
static QueueHandle_t reporter_queue_handle;
static StaticQueue_t reporter_queue_control_block;
static uint8_t queue_storage[sizeof(reporter_report_t)];

// Report
static reporter_report_t reporter_report = {.forward_duty_cycle = 0,
                                            .left_duty_cycle = 0,
                                            .right_duty_cycle = 0,
                                            .reverse_duty_cycle = 0,
                                            .hand_brake = false};

// PWM
TickType_t reporter_pwm_period;
const uint32_t reporter_max_reports_before_reset_attempt = 1000;

void reporter_init(TickType_t pwm_period) {
    reporter_pwm_period = pwm_period;
    reporter_queue_handle =
        xQueueCreateStatic(1, sizeof(reporter_report_t), queue_storage, &reporter_queue_control_block);
}

void reporter_set_report(const reporter_report_t* command) {
    xQueueOverwrite(reporter_queue_handle, command);
}

void reporter_task(void* unused) {
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
                reporter_report_t new_report;
                if (pdPASS == xQueueReceive(reporter_queue_handle, &new_report, 0)) {
                    reporter_report = new_report;
                }
            }
            const float elapsed_fraction = (float)elapsed / (float)reporter_pwm_period;

            // Create HID report
            uint8_t key_codes[6] = {0x00};
            uint8_t key_codes_added = 0;

            if (elapsed_fraction < reporter_report.forward_duty_cycle) {
                key_codes[key_codes_added] = SCAN_CODE_W;
                key_codes_added++;
            }
            if (elapsed_fraction < reporter_report.left_duty_cycle) {
                key_codes[key_codes_added] = SCAN_CODE_A;
                key_codes_added++;
            }
            if (elapsed_fraction < reporter_report.right_duty_cycle) {
                key_codes[key_codes_added] = SCAN_CODE_D;
                key_codes_added++;
            }
            if (elapsed_fraction < reporter_report.reverse_duty_cycle) {
                key_codes[key_codes_added] = SCAN_CODE_S;
                key_codes_added++;
            }
            if (true) {
                key_codes[key_codes_added] = SCAN_CODE_SPACEBAR;
                key_codes_added++;
            }

            // TODO do not send an empty report if a single shot key has been pressed

            if (reporter_max_reports_before_reset_attempt <= n_reports_sent) {
                // Every N reports send an empty one to unfuck stuck keys
                uint8_t zeros[8] = {0};
                tud_hid_keyboard_report(0, 0, zeros);
                n_reports_sent = 0;
            } else {
                // Otherwise, send the report
                tud_hid_keyboard_report(0, 0, key_codes);
                n_reports_sent++;
            }
        }

        vTaskDelayUntil(&wake_time, reporter_interval);
    }
}

// Starts the reporter task.
void reporter_start(UBaseType_t priority, TickType_t interval) {
    reporter_interval = interval;
    xTaskCreateStatic(reporter_task, "reporter Task", reporter_TASK_STACK_SIZE, NULL, priority, reporter_task_stack,
                      &reporter_task_control_block);
}
