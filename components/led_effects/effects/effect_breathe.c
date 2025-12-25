#include "led_effects.h"
#include "led_topology.h"
#include "ws2812.h"

/* Breathing effect state */
typedef struct
{
    float phase;
    float speed; // cycles per second
    uint8_t r, g, b;
} effect_breathe_ctx_t;

void effect_breathe(
    led_topology_t *unused,
    effect_time_t *time,
    void *user_ctx)
{
    (void)unused;

    effect_breathe_ctx_t *ctx = (effect_breathe_ctx_t *)user_ctx;

    /* Advance phase */
    float delta_sec = time->delta_ms / 1000.0f;
    ctx->phase += ctx->speed * delta_sec;

    if (ctx->phase >= 1.0f)
        ctx->phase -= 1.0f;

    /* Triangle wave */
    float level = (ctx->phase < 0.5f)
                      ? (ctx->phase * 2.0f)
                      : ((1.0f - ctx->phase) * 2.0f);

    uint8_t r = (uint8_t)(ctx->r * level);
    uint8_t g = (uint8_t)(ctx->g * level);
    uint8_t b = (uint8_t)(ctx->b * level);

    uint16_t total = led_topology_total_leds();

    for (uint16_t logical = 0; logical < total; logical++)
    {
        uint16_t physical = led_topology_map(logical);
        ws2812_set_pixel(physical, r, g, b);
    }
}
