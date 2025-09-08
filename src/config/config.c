#include "config.h"

#include <hardware/flash.h>
#include <hardware/regs/addressmap.h>
#include <hardware/sync.h>
#include <string.h>

#include "FreeRTOS.h"
#include "pins.h"
#include "semphr.h"
#include "terminal/terminal.h"

// Logging
const char* const config_logging_tag = "CONFIG";

// Mutex
StaticSemaphore_t config_mutex;
SemaphoreHandle_t config_mutex_handle;

// Config
#define CONFIG_MAGIC 0x5AD00DAD
#define CONFIG_CURRENT_VERSION 1

typedef struct config {
    uint32_t magic;
    uint32_t version;
    uint32_t size;  // Set to sizeof(config_t), supplements version for avoiding bad config reads

    bool calibration_set;
    control_raw_report_t calibration_max;
    control_raw_report_t calibration_min;

    bool control_settings_set;
    control_settings_t control_settings;
} config_t;

static config_t config;

config_t* config_in_flash(void) {
    return (config_t*)(XIP_BASE + FLASH_LAST_SECTOR_OFFSET);
}

void config_init(void) {
    // Try to read from flash
    config = *(config_in_flash());

    // Validate config
    bool config_valid = true;
    if (CONFIG_MAGIC != config.magic) {
        LOG_W(config_logging_tag, "Config magic value does not match value in flash.");
        config_valid = false;
    } else if (CONFIG_CURRENT_VERSION != config.version) {
        LOG_W(config_logging_tag, "Config version does not match value in flash.");
        config_valid = false;
    } else if (sizeof(config_t) != config.size) {
        LOG_W(config_logging_tag, "Config size value does not match value in flash.");
        config_valid = false;
    }

    // Init config, if required
    if (!config_valid) {
        memset(&config, 0, sizeof(config_t));
        config.magic = CONFIG_MAGIC;
        config.version = CONFIG_CURRENT_VERSION;
        config.size = sizeof(config_t);
    }

    // Set up mutex
    config_mutex_handle = xSemaphoreCreateMutexStatic(&config_mutex);
}

static void __no_inline_not_in_flash_func(config_save_to_flash_impl)(void) {
    disable_interrupts();
    flash_range_erase(FLASH_LAST_SECTOR_OFFSET, FLASH_LAST_SECTOR_SIZE);
    flash_range_program(FLASH_LAST_SECTOR_OFFSET, (uint8_t*)&config, sizeof(config_t));
    enable_interrupts();
}

static void config_save_to_flash(void) {
    if (0 == memcmp(&config, config_in_flash(), sizeof(config_t))) {
        LOG_D(config_logging_tag, "Skipping flash write as it would have no effect.");
        return;
    }
    config_save_to_flash_impl();
    TANK_ASSERT(0 == memcmp((uint8_t*)config_in_flash(), (uint8_t*)&config, sizeof(config_t)));
    LOG_D(config_logging_tag, "Wrote config to flash.");
}

void config_set_calibration(const control_raw_report_t* min, const control_raw_report_t* max) {
    TANK_ASSERT(pdTRUE == xSemaphoreTake(config_mutex_handle, portMAX_DELAY));
    config.calibration_set = true;
    config.calibration_min = *min;
    config.calibration_max = *max;
    config_save_to_flash();
    TANK_ASSERT(pdTRUE == xSemaphoreGive(config_mutex_handle));
}

bool config_get_calibration(control_raw_report_t* min, control_raw_report_t* max) {
    TANK_ASSERT(pdTRUE == xSemaphoreTake(config_mutex_handle, portMAX_DELAY));
    if (!config.calibration_set) {
        TANK_ASSERT(pdTRUE == xSemaphoreGive(config_mutex_handle));
        return false;
    }
    *min = config.calibration_min;
    *max = config.calibration_max;

    TANK_ASSERT(pdTRUE == xSemaphoreGive(config_mutex_handle));
    return true;
}

void config_set_control_settings(const control_settings_t* settings) {
    TANK_ASSERT(pdTRUE == xSemaphoreTake(config_mutex_handle, portMAX_DELAY));
    config.control_settings_set = true;
    config.control_settings = *settings;
    config_save_to_flash();
    TANK_ASSERT(pdTRUE == xSemaphoreGive(config_mutex_handle));
}

bool config_get_control_settings(control_settings_t* settings) {
    TANK_ASSERT(pdTRUE == xSemaphoreTake(config_mutex_handle, portMAX_DELAY));
    if (!config.control_settings_set) {
        TANK_ASSERT(pdTRUE == xSemaphoreGive(config_mutex_handle));
        return false;
    }
    *settings = config.control_settings;
    TANK_ASSERT(pdTRUE == xSemaphoreGive(config_mutex_handle));
    return true;
}