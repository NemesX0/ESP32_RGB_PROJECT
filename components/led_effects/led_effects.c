#include "led_effects.h"

static led_topology_t *s_topo = NULL;
static const led_effect_t *s_current = NULL;

static uint32_t s_last_ms = 0;

esp_err_t led_effects_init(led_topology_t *topology)
{
    if (!topology)
        return ESP_ERR_INVALID_ARG;
    s_topo = topology;
    s_last_ms = 0;
    return ESP_OK;
}

esp_err_t led_effects_set(const led_effect_t *effect)
{
    if (!effect || !effect->render)
        return ESP_ERR_INVALID_ARG;
    s_current = effect;
    return ESP_OK;
}

void led_effects_tick(uint32_t now_ms)
{
    if (!s_topo || !s_current)
        return;

    effect_time_t t = {
        .now_ms = now_ms,
        .delta_ms = (s_last_ms == 0) ? 0 : (now_ms - s_last_ms),
        .brightness = 255};

    s_last_ms = now_ms;

    s_current->render(s_topo, &t, s_current->user_ctx);
}
