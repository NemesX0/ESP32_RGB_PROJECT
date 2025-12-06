#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>
#include "hal/gpio_types.h"

// Initialize WS2812 strip using RMT Encoder API.
// gpio: data pin for the strip
// led_count: number of LEDs in the strip
esp_err_t ws2812_init(gpio_num_t gpio, uint32_t led_count);

// Deinit and free resources
void ws2812_deinit(void);

// Set a single pixel in the internal buffer (0-based index)
// NOTE: This only updates RAM, you must call ws2812_show() to send to LEDs.
void ws2812_set_pixel(uint32_t index, uint8_t r, uint8_t g, uint8_t b);

// Fill the entire strip with a color (RAM only, you must call ws2812_show)
void ws2812_fill(uint8_t r, uint8_t g, uint8_t b);

// Clear (set all pixels to 0,0,0) and keep in RAM (call show to apply)
void ws2812_clear(void);

// Push current frame buffer to the LEDs
esp_err_t ws2812_show(void);

// Get current LED count
uint32_t ws2812_get_count(void);
