#include "fs.h"
#include "config_system.h"
#include "ws2812.h"
#include "led_effects.h"
#include "led_topology.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Forward declaration of the effect */
void effect_breathe(
    led_topology_t *topo,
    effect_time_t *time,
    void *user_ctx);

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

    /* --- WS2812 init --- */
    err = ws2812_init((gpio_num_t)cfg->led_gpio, cfg->led_count);
    if (err != ESP_OK)
    {
        ESP_LOGE("MAIN", "WS2812 init FAILED: %s", esp_err_to_name(err));
        return;
    }

    /* --- Stage 5: Topology Engine (RUNTIME init, not const) --- */
    led_strip_t strips[1];
    strips[0].led_count = cfg->led_count;
    strips[0].reversed = false;

    led_topology_t topology;
    topology.strip_count = 1;
    topology.strips = strips;

    led_topology_init(&topology);

    /* --- Stage 6: Effects Engine --- */
    led_effects_init(&topology);

    static struct
    {
        float phase;
        float speed;
        uint8_t r, g, b;
    } breathe_ctx = {
        .phase = 0.0f,
        .speed = 0.5f,
        .r = 0,
        .g = 0,
        .b = 255};

    static const led_effect_t breathe_effect = {
        .name = "breathe_blue",
        .render = effect_breathe,
        .user_ctx = &breathe_ctx};

    led_effects_set(&breathe_effect);

    /* --- Main loop --- */
    while (true)
    {
        uint32_t now_ms = esp_timer_get_time() / 1000;

        led_effects_tick(now_ms);

        /* Use the REAL ws2812 API */
        ws2812_show();

        vTaskDelay(pdMS_TO_TICKS(10)); // ~100 FPS
    }
}
