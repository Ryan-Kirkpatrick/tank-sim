#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "control/types.h"

// Init config
void config_init(void);

// Set the calibration and save it to flash.
void config_set_calibration(const control_raw_report_t* min, const control_raw_report_t* max);

// Try to get calibration settings from configuration. Returns true on success, false on error.
bool config_get_calibration(control_raw_report_t* min, control_raw_report_t* max);

// Set the control settings and save them to flash.
void config_set_control_settings(const control_settings_t* settings);

// Try to get control settings from configuration. Returns true on success, false on error.
bool config_get_control_settings(control_settings_t* settings);