#include <hardware/gpio.h>
#include <pico/time.h>
#include <stdint.h>
#include <string.h>

#include "hx710c.h"

// Delay approximately 150ns (close enough for government work)
__attribute__((always_inline)) static inline void hx710c_delay_150ns() {
    asm volatile(
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n"
        "nop\n");
}

// Delay approximately 300ns (close enough for government work)
__attribute__((always_inline)) static inline void hx710c_delay_300ns() {
    hx710c_delay_150ns();
    hx710c_delay_150ns();
}

void hx710c_init(hx710c_t* const device, const uint8_t scl_pin, const uint8_t* const sda_pins,
                 const uint8_t n_sda_pins) {
    // Fill struct
    device->n_sda_pins = n_sda_pins;
    device->scl_pin = scl_pin;
    device->sda_pins = sda_pins;

    // Init SCL
    gpio_init(scl_pin);
    gpio_disable_pulls(scl_pin);
    gpio_set_dir(scl_pin, true);

    // Init SDA pins
    for (uint8_t i = 0; i < n_sda_pins; i++) {
        gpio_init(sda_pins[i]);
        gpio_disable_pulls(sda_pins[i]);
        gpio_set_dir(sda_pins[i], false);
    }

    // Select 40hz sample rate
    gpio_put(scl_pin, false);
    hx710c_delay_150ns();  // T1

    const uint8_t n_pulses_for_40hz = 27;
    for (uint8_t i = 0; i < n_pulses_for_40hz; i++) {
        gpio_put(scl_pin, true);
        hx710c_delay_300ns();  // T3
        gpio_put(scl_pin, false);
        hx710c_delay_300ns();  // T4
    }
}

bool hx710c_read(hx710c_t* device, int32_t* conversions) {
    // Check ready
    if (!hx710c_is_ready(device)) {
        return false;
    }

    // Init conversions
    memset(conversions, 0, device->n_sda_pins * sizeof(uint32_t));
    uint32_t* conversions_u32 = (uint32_t*)conversions;

    // Ensure T1 timing
    hx710c_delay_150ns();

    // Read conversion
    for (int8_t bit = 23; bit > -1; bit--) {
        gpio_put(device->scl_pin, true);
        hx710c_delay_150ns();  // T2
        for (uint8_t i = 0; i < device->n_sda_pins; i++) {
            conversions_u32[i] |= ((uint32_t)gpio_get(device->sda_pins[i])) << bit;
        }
        hx710c_delay_300ns();  // T3
        gpio_put(device->scl_pin, false);
        hx710c_delay_300ns();  // T4
    }

    // Send three more clock pulses to select 40hz mode again.
    for (uint8_t i = 0; i < 3; i++) {
        gpio_put(device->scl_pin, true);
        hx710c_delay_300ns();  // T3
        gpio_put(device->scl_pin, false);
        hx710c_delay_300ns();  // T4
    }

    // Convert conversions from 24bit twos complement to int32_t
    for (uint8_t i = 0; i < device->n_sda_pins; i++) {
        if (conversions_u32[i] & 0x800000) {
            conversions[i] = (int32_t)(conversions_u32[i] | 0xFF000000);
        } else {
            conversions[i] = (int32_t)(conversions_u32[i] & 0x00FFFFFF);
        }
    }

    return true;
}

bool hx710c_is_ready(hx710c_t* device) {
    // A conversion is ready if the SDA line is low
    for (uint i = 0; i < device->n_sda_pins; i++) {
        if (gpio_get(device->sda_pins[i])) {
            return false;
        }
    }
    return true;
}
