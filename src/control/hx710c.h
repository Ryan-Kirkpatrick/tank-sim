#pragma ONCE

#include <stdint.h>
#include <stdbool.h>

typedef struct hx710c {
    uint8_t scl_pin;
    const uint8_t* sda_pins;
    uint8_t n_sda_pins;
} hx710c_t;

// Initializes the HX710C devices on the specified pins, filling `device` and 
// perfroming hardware initialisation.
// The SDA pins buffer must have the same life time as the hx710 device
//
// Not RTOS aware, the caller must ensure this function is not prememted.
void hx710c_init(hx710c_t* device, uint8_t scl_pin, const uint8_t* sda_pins, uint8_t n_sda_pins);

// Performs an ADC conversion.
// `conversions` is a buffer that can hold at least `n_sda_pins` results.
//
// Calling this function faster that the conversion rate of HX710 will cause the
// devices to behave incorrectly. It is the callers responsibility to ensure
// this does not occur. The maximum sampling rate is 40hz.
// The maximum sampling rate of the HX710C is 40Hz. This function checks if a
// sample is ready before attempting to read. If a sample is not ready then
// the function will return false.
//
// Not RTOS aware, the caller must ensure this function is not prememted.
//
// Returns true on success or false if no data was available.
bool hx710c_read(hx710c_t *device, int32_t *conversions);

// Returns true if devices are ready to be read.
bool hx710c_is_ready(hx710c_t *device);


