#pragma once

#include <hardware/flash.h>
#include <stdint.h>
#include <stdio.h>

#include "util/tank_assert.h"

// LEDS
#define LED_PIN 25

// Potentiometer
#define POT_PIN 26

// Pedals
#define CLUTCH_PEDAL_PIN 28
#define BRAKE_PEDAL_PIN 27
#define ACCELERATOR_PEDAL_PIN 26

// Tillers
#define LEFT_TILLER_SDA_PIN 19
#define RIGHT_TILLER_SDA_PIN 20
#define TILLER_SCL_PIN 21

// Calibration switch
#define CALIBRATION_SWITCH_PIN 15

// Engine on/off switch
#define ENGINE_ON_OFF_SWITCH_PIN 14

// Stdio
#define STDIO_UART_BAUD_RATE 115200
#define STDIO_UART_ID uart0
#define STDIO_UART_TX_PIN 0
#define STDIO_UART_RX_PIN 1

// ADC
#define PIN_TO_ADC(x) _PIN_TO_ADC((x))
static inline uint8_t _PIN_TO_ADC(uint8_t pin) {
    const uint8_t adc = pin - 26;
    TANK_ASSERT(adc < 3);
    return adc;
}

// flash
#define FLASH_LAST_SECTOR_SIZE FLASH_SECTOR_SIZE
#define FLASH_LAST_SECTOR_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)