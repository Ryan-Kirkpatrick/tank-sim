#include "input_task.h"
#include <hardware/gpio.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "hardware/adc.h"
#include "helpers.h"
#include "hx710c.h"
#include "pico/stdlib.h"
#include "pins.h"
#include "portmacro.h"
#include "projdefs.h"
#include "stdio.h"
#include "tank_assert.h"
#include "task.h"

// Stack info
#define INPUT_TASK_STACK_SIZE 4096 / sizeof(StackType_t)
static StackType_t input_task_stack[INPUT_TASK_STACK_SIZE];
static StaticTask_t input_task_control_block;

// Globals
static TickType_t input_interval = 0;
static hx710c_t input_force_sensors;

// Raw input
#define INPUT_RAW_PEDAL_ABSOLUTE_MIN ((uint16_t)0)
#define INPUT_RAW_PEDAL_ABSOLUTE_MAX ((uint16_t)4095)

#define INPUT_RAW_TILLER_ABSOLUTE_MIN ((int32_t)-8388608)
#define INPUT_RAW_TILLER_ABSOLUTE_MAX ((int32_t)8388607)

typedef struct input_raw_report {
    int16_t accelerator;
    int16_t brake;
    int16_t clutch;
    int32_t left_tiller;
    int32_t right_tiller;
} input_raw_report_t;

const char* input_gear_to_str(input_gear_t gear) {
    switch (gear) {
        case INPUT_FORWARDS:
            return "INPUT_FORWARDS";
        case INPUT_REVERSE:
            return "INPUT_REVERSE";
    }
    TANK_ASSERT_M(false, "Unexpected input_gear_t");
    return "";
}

void input_report_print(const input_report_t* report) {
    printf("{\r\n");
    printf("  \"left_tiller\": %f,\r\n", report->left_tiller);
    printf("  \"right_tiller\": %f,\r\n", report->right_tiller);
    printf("  \"accelerator\": %f,\r\n", report->accelerator);
    printf("  \"gear\": %s\r\n", input_gear_to_str(report->gear));
    printf("}\r\n");
}

static input_raw_report_t input_task_read_sensors(void) {
    input_raw_report_t report = {0};

    // Read force sensors
    uint32_t conversions[2] = {0};
    vTaskSuspendAll();
    bool conversion_ready = hx710c_read(&input_force_sensors, conversions);
    xTaskResumeAll();
    if (!conversion_ready) {
        printf("Force sensor conversion was not ready.\r\n");
    }
    report.left_tiller = conversions[0];
    report.right_tiller = conversions[1];

    // Read pedals
    adc_select_input(PIN_TO_ADC(ACCELERATOR_PEDAL_PIN));
    report.accelerator = adc_read();
    adc_select_input(PIN_TO_ADC(BRAKE_PEDAL_PIN));
    report.brake = adc_read();
    adc_select_input(PIN_TO_ADC(CLUTCH_PEDAL_PIN));
    report.clutch = adc_read();

    return report;
}

static bool input_calibration_mode_enabled(void) {
    static bool enabled = false;
    static bool enabled_last_call = false;
    static TickType_t last_read = 0;
    const static TickType_t minimum_read_interval = pdMS_TO_TICKS(200);  // For debouncing

    // Do not read more often than the minimum interval
    TickType_t current_tick = xTaskGetTickCount();
    if (current_tick - last_read < minimum_read_interval) {
        return enabled;
    }

    // Read calibration switch
    enabled = !gpio_get(CALIBRATION_SWITCH_PIN);

    // Log
    if (enabled && !enabled_last_call) {
        printf("Entering calibration mode.\r\n");
    }
    if (!enabled && enabled_last_call) {
        printf("Leaving calibration mode.\r\n");
    }

    enabled_last_call = enabled;
    return enabled;
}

// Updates max and min based off the current sensor reading
static void input_calibrate(const input_raw_report_t* current, input_raw_report_t* min, input_raw_report_t* max) {
    // Update min
    min->accelerator = MIN_OF(min->accelerator, current->accelerator);
    min->brake = MIN_OF(min->brake, current->brake);
    min->clutch = MIN_OF(min->clutch, current->clutch);
    min->left_tiller = MIN_OF(min->left_tiller, current->left_tiller);
    min->right_tiller = MIN_OF(min->right_tiller, current->right_tiller);

    // Update max
    max->accelerator = MAX_OF(max->accelerator, current->accelerator);
    max->brake = MAX_OF(max->brake, current->brake);
    max->clutch = MAX_OF(max->clutch, current->clutch);
    max->left_tiller = MAX_OF(max->left_tiller, current->left_tiller);
    max->right_tiller = MAX_OF(max->right_tiller, current->right_tiller);
}

// Scales raw values to between 0.0 and 1.0
static float input_scale_to_0_1(float current_raw, float min_raw, float max_raw) {
    // Do not divide by zero
    if (max_raw == min_raw) {
        return 0;
    }

    float value = (current_raw - min_raw) / (max_raw - min_raw);
    value = MAX_OF(value, 0.0);
    value = MIN_OF(value, 1.0);
    return value;
}

static input_report_t input_make_report(const input_raw_report_t* current, const input_raw_report_t* calibration_min,
                                        const input_raw_report_t* calibration_max) {
    input_report_t report = {
        // High raw accelerator values indicate that the accelerator is not depressed
        .accelerator =
            1.0 - input_scale_to_0_1(current->accelerator, calibration_min->accelerator, calibration_max->accelerator),
        .left_tiller =
            input_scale_to_0_1(current->left_tiller, calibration_min->left_tiller, calibration_max->left_tiller),
        .right_tiller =
            input_scale_to_0_1(current->right_tiller, calibration_min->right_tiller, calibration_max->right_tiller),
        // TODO gear selection
        .gear = INPUT_FORWARDS};

    return report;
}

static void input_task(void* unused) {
    TickType_t wake_time = xTaskGetTickCount();

    // Set up force sensors
    // Setup force sensors
    const uint8_t force_sensor_sda_pins[] = {
        LEFT_TILLER_SDA_PIN,
        RIGHT_TILLER_SDA_PIN,
    };
    vTaskSuspendAll();
    hx710c_init(&input_force_sensors, TILLER_SCL_PIN, force_sensor_sda_pins, 2);
    xTaskResumeAll();

    const input_raw_report_t default_report = {
        .accelerator = INPUT_RAW_PEDAL_ABSOLUTE_MAX,  //
        .brake = INPUT_RAW_PEDAL_ABSOLUTE_MAX,        //
        .clutch = INPUT_RAW_PEDAL_ABSOLUTE_MAX,       //
        .left_tiller = 0,                             //
        .right_tiller = 0                             //
    };

    input_raw_report_t calibration_min = {
        //The minimum values found in the calibration step
        .accelerator = INPUT_RAW_PEDAL_ABSOLUTE_MAX,   //
        .brake = INPUT_RAW_PEDAL_ABSOLUTE_MAX,         //
        .clutch = INPUT_RAW_PEDAL_ABSOLUTE_MAX,        //
        .left_tiller = INPUT_RAW_TILLER_ABSOLUTE_MAX,  //
        .right_tiller = INPUT_RAW_TILLER_ABSOLUTE_MAX  //
    };

    input_raw_report_t calibration_max = {
        // The maximum values found in the calibration step
        .accelerator = INPUT_RAW_PEDAL_ABSOLUTE_MIN,   //
        .brake = INPUT_RAW_PEDAL_ABSOLUTE_MIN,         //
        .clutch = INPUT_RAW_PEDAL_ABSOLUTE_MIN,        //
        .left_tiller = INPUT_RAW_TILLER_ABSOLUTE_MIN,  //
        .right_tiller = INPUT_RAW_TILLER_ABSOLUTE_MIN  //
    };

    while (1) {
        // Read sensors
        input_raw_report_t current_report = input_task_read_sensors();

        if (input_calibration_mode_enabled()) {
            input_calibrate(&current_report, &calibration_min, &calibration_max);
        } else {
            input_report_t final_report = input_make_report(&current_report, &calibration_min, &calibration_max);
            input_report_print(&final_report);
        }

        vTaskDelayUntil(&wake_time, input_interval);
    }
}

void input_task_start(UBaseType_t priority, TickType_t interval) {
    input_interval = interval;
    xTaskCreateStatic(input_task, "Input Task", INPUT_TASK_STACK_SIZE, NULL, priority, input_task_stack,
                      &input_task_control_block);
}

void input_task_init(void) {
    // Setup pedals
    adc_init();
    adc_gpio_init(ACCELERATOR_PEDAL_PIN);
    adc_gpio_init(BRAKE_PEDAL_PIN);
    adc_gpio_init(CLUTCH_PEDAL_PIN);

    // Setup calibration switch
    gpio_init(CALIBRATION_SWITCH_PIN);
    gpio_set_dir(CALIBRATION_SWITCH_PIN, false);
    gpio_pull_up(CALIBRATION_SWITCH_PIN);
}
