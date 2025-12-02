#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    bool provisioned;   // has first-time setup been completed?
    int led_gpio;       // GPIO pin for WS2812
    uint32_t led_count; // number of LEDs

    char device_name[32]; // internal name
    char ble_name[32];    // BLE advertised name
    char ap_password[32]; // Wi-Fi AP password (for setup/OTA)
    char ble_pin[8];      // 6-digit PIN + null (e.g. "123456")
} system_config_t;

// Initialize system config (must be called AFTER fs_init())
esp_err_t system_config_init(void);

// Get pointer to in-memory config (read-only for now)
const system_config_t *system_config_get(void);

// Mark device provisioned and save
esp_err_t system_config_set_provisioned(bool value);

// Save current config to /sys/system.json
esp_err_t system_config_save(void);

// Factory reset: reset to defaults and save
esp_err_t system_config_factory_reset(void);

// Convenience helpers
bool system_config_is_provisioned(void);
int system_config_get_led_gpio(void);
uint32_t system_config_get_led_count(void);
const char *system_config_get_ble_name(void);
const char *system_config_get_device_name(void);
