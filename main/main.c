

#include "fs.h"
#include "config_system.h"
#include "ws2812.h"

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


void app_main(void)
{
    esp_err_t err;

    err = fs_init();
    if (err != ESP_OK)
    {
        ESP_LOGE("MAIN", "FS init FAILED: %s", esp_err_to_name(err));
        return;
    }

    err = system_config_init();
    if (err != ESP_OK)
    {
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

    // --- Stage 3: WS2812 test ---

    err = ws2812_init((gpio_num_t)cfg->led_gpio, cfg->led_count);
    if (err != ESP_OK)
    {
        ESP_LOGE("MAIN", "WS2812 init FAILED: %s", esp_err_to_name(err));
        return;
    }
    
    while (true)
    {
    vTaskDelay(pdMS_TO_TICKS(1000));
    }

}
