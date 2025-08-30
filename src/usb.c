#include "usb.h"
#include "portmacro.h"
#include "task.h"
#include "tusb.h"

#define USB_TASK_STACK_SIZE 1024 / sizeof(StackType_t)
static StackType_t usb_task_stack[USB_TASK_STACK_SIZE];
static StaticTask_t usb_task_control_block;

static TickType_t usb_interval = 0;

static void usb_task(void* unused) {
    tusb_init();  // Must be called after the task scheduler is running

    TickType_t wake_time = xTaskGetTickCount();
    while (1) {
        tud_task();
        vTaskDelayUntil(&wake_time, usb_interval);
    }
}

void usb_start(UBaseType_t priority, TickType_t interval) {
    usb_interval = interval;
    xTaskCreateStatic(usb_task, "USB Task", USB_TASK_STACK_SIZE, NULL, priority, usb_task_stack,
                      &usb_task_control_block);
}

void usb_init(void) {
    // Nop
}