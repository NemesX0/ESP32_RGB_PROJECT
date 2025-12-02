#include "fs.h"
#include "config_system.h"
#include "esp_log.h"
#include "esp_err.h"

void app_main(void)
{
    esp_err_t err;

    err = fs_init();
    if (err != ESP_OK) {
        ESP_LOGE("MAIN", "FS init FAILED: %s", esp_err_to_name(err));
        return;
    }

    err = system_config_init();
    if (err != ESP_OK) {
        ESP_LOGE("MAIN", "System config init FAILED: %s", esp_err_to_name(err));
        return;
    }

    const system_config_t *cfg = system_config_get();

    ESP_LOGI("MAIN", "Config: provisioned=%d, gpio=%d, leds=%lu, dev='%s', ble='%s'",
             cfg->provisioned,
             cfg->led_gpio,
             (unsigned long)cfg->led_count,
             cfg->device_name,
             cfg->ble_name);
}
