#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "led_topology.h"

typedef struct
{
    uint32_t now_ms;
    uint32_t delta_ms;
    uint8_t brightness; // 0–255
} effect_time_t;

/* Effect function signature */
typedef void (*led_effect_fn_t)(
    led_topology_t *topo,
    effect_time_t *time,
    void *user_ctx);

/* Effect descriptor */
typedef struct
{
    const char *name;
    led_effect_fn_t render;
    void *user_ctx;
} led_effect_t;

/* Engine control */
esp_err_t led_effects_init(led_topology_t *topology);
esp_err_t led_effects_set(const led_effect_t *effect);
void led_effects_tick(uint32_t now_ms);
