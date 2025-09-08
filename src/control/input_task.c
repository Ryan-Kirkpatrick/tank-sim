#include "input_task.h"

#include <hardware/gpio.h>
#include <pico/stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "config/config.h"
#include "control_map.h"
#include "hardware/adc.h"
#include "hx710c.h"
#include "pins.h"
#include "projdefs.h"
#include "task.h"
#include "terminal/terminal.h"
#include "usb_keyboard/keyboard_task.h"
#include "usb_keyboard/types.h"
#include "util/helpers.h"
#include "util/tank_assert.h"

// Logging
static const char* const input_log_tag = "INPUT";

// Stack info
#define INPUT_TASK_STACK_SIZE 4096 / sizeof(StackType_t)
static StackType_t input_task_stack[INPUT_TASK_STACK_SIZE];
static StaticTask_t input_task_control_block;

// Globals
static TickType_t input_interval = 0;
static hx710c_t input_force_sensors;

// Calibration
const control_raw_report_t control_default_calibration_min = {
    //The minimum values found in the calibration step
    .accelerator = INPUT_RAW_PEDAL_ABSOLUTE_MAX,  //
    .brake = INPUT_RAW_PEDAL_ABSOLUTE_MAX,        //
    .clutch = INPUT_RAW_PEDAL_ABSOLUTE_MAX,       //
    .left_tiller = INT16_MAX,                     //
    .right_tiller = INT16_MAX                     //
};

const control_raw_report_t control_default_calibration_max = {
    // The maximum values found in the calibration step
    .accelerator = INPUT_RAW_PEDAL_ABSOLUTE_MIN,  //
    .brake = INPUT_RAW_PEDAL_ABSOLUTE_MIN,        //
    .clutch = INPUT_RAW_PEDAL_ABSOLUTE_MIN,       //
    .left_tiller = INT16_MIN,                     //
    .right_tiller = INT16_MIN                     //
};

// Returns true if all sensors have bene successfully read
static bool input_task_update_sensor_values(control_raw_report_t* sensor_values) {
    bool result = true;

    // Read force sensors
    int32_t conversions[2] = {0};
    vTaskSuspendAll();
    bool conversion_ready = hx710c_read(&input_force_sensors, conversions);
    xTaskResumeAll();
    if (!conversion_ready) {
        LOG_W(input_log_tag, "Force sensor conversion was not ready.");
        result = false;
    } else {
        sensor_values->left_tiller = conversions[0];
        sensor_values->right_tiller = conversions[1];
    }

    // Read pedals
    adc_select_input(PIN_TO_ADC(ACCELERATOR_PEDAL_PIN));
    sensor_values->accelerator = adc_read();
    adc_select_input(PIN_TO_ADC(BRAKE_PEDAL_PIN));
    sensor_values->brake = adc_read();
    adc_select_input(PIN_TO_ADC(CLUTCH_PEDAL_PIN));
    sensor_values->clutch = adc_read();

    return result;
}

static bool input_calibration_mode_enabled(bool* out_exited_calibration_mode, bool* out_entered_calibration_mode) {
    static bool enabled = false;
    static bool enabled_last_call = false;
    static TickType_t last_read = 0;
    const static TickType_t minimum_read_interval = pdMS_TO_TICKS(200);  // For debouncing
    if (NULL != out_exited_calibration_mode) {
        *out_exited_calibration_mode = false;
    }
    if (NULL != out_entered_calibration_mode) {
        *out_entered_calibration_mode = false;
    }

    // Do not read more often than the minimum interval
    TickType_t current_tick = xTaskGetTickCount();
    if (current_tick - last_read < minimum_read_interval) {
        return enabled;
    }

    // Read calibration switch
    enabled = !gpio_get(CALIBRATION_SWITCH_PIN);

    // Log
    if (enabled && !enabled_last_call) {
        LOG_I(input_log_tag, "Entering calibration mode.");
        if (NULL != out_entered_calibration_mode) {
            *out_entered_calibration_mode = true;
        }
    }
    if (!enabled && enabled_last_call) {
        LOG_I(input_log_tag, "Leaving calibration mode.");
        if (NULL != out_exited_calibration_mode) {
            *out_exited_calibration_mode = true;
        }
    }

    enabled_last_call = enabled;
    return enabled;
}

// Updates max and min based off the current sensor reading
static void input_calibrate(const control_raw_report_t* current, control_raw_report_t* min, control_raw_report_t* max) {
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

static input_report_t input_make_report(const control_raw_report_t* current,
                                        const control_raw_report_t* calibration_min,
                                        const control_raw_report_t* calibration_max) {
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

    control_raw_report_t calibration_min = control_default_calibration_min;
    control_raw_report_t calibration_max = control_default_calibration_max;
    // Control settings
    control_settings_t control_settings = {.pedal_deadzone = 0.07,
                                           .tiller_deadzone = 0.07,
                                           .tiller_handbrake_threshold_begin = 0.8,
                                           .tiller_handbrake_threshold_end = 0.9,
                                           .tiller_max_turn_threshold = 0.65};

    // Read from config
    config_get_calibration(&calibration_min, &calibration_max);
    config_get_control_settings(&control_settings);

    // Init current report
    control_raw_report_t current_report = {
        .accelerator = INPUT_RAW_PEDAL_ABSOLUTE_MAX,  //
        .brake = INPUT_RAW_PEDAL_ABSOLUTE_MAX,        //
        .clutch = INPUT_RAW_PEDAL_ABSOLUTE_MAX,       //
        .left_tiller = 0,                             //
        .right_tiller = 0                             //
    };
    while (!input_task_update_sensor_values(&current_report)) {
        // NOP
    }

    while (1) {
        // Read sensors
        input_task_update_sensor_values(&current_report);

        // Process sensor data
        bool save_calibration_data = false;
        bool reset_calibration_data = false;
        if (input_calibration_mode_enabled(&save_calibration_data, &reset_calibration_data)) {
            if (reset_calibration_data) {
                LOG_D(input_log_tag, "Reset calibration data.");
                control_raw_report_t calibration_min = control_default_calibration_min;
                control_raw_report_t calibration_max = control_default_calibration_max;
            }
            keyboard_output_t nil_output = {0};
            keyboard_task_set_output(&nil_output);
            input_calibrate(&current_report, &calibration_min, &calibration_max);
        } else {
            input_report_t input = input_make_report(&current_report, &calibration_min, &calibration_max);
            keyboard_output_t output = map_input_to_output(&control_settings, &input);
            keyboard_task_set_output(&output);
        }

        // Save calibration data
        if (save_calibration_data) {
            config_set_calibration(&calibration_min, &calibration_max);
            LOG_D(input_log_tag, "Calibration minimums:");
            LOG_D(input_log_tag, "  accelerator: %d", calibration_min.accelerator);
            LOG_D(input_log_tag, "  left_tiller: %d", calibration_min.left_tiller);
            LOG_D(input_log_tag, "  right_tiller: %d", calibration_min.right_tiller);
            LOG_D(input_log_tag, "Calibration maximums:");
            LOG_D(input_log_tag, "  accelerator: %d", calibration_max.accelerator);
            LOG_D(input_log_tag, "  left_tiller: %d", calibration_max.left_tiller);
            LOG_D(input_log_tag, "  right_tiller: %d", calibration_max.right_tiller);
            LOG_D(input_log_tag, "Current reading:");
            LOG_D(input_log_tag, "  accelerator: %d", current_report.accelerator);
            LOG_D(input_log_tag, "  left_tiller: %d", current_report.left_tiller);
            LOG_D(input_log_tag, "  right_tiller: %d", current_report.right_tiller);
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
