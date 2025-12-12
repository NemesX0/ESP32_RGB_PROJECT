#include "led_topology.h"
#include "esp_log.h"

static const char *TAG = "led_topology";

static const led_topology_t *s_topology = NULL;
static uint16_t s_total_leds = 0;

void led_topology_init(const led_topology_t *topo)
{
    s_topology = topo;
    s_total_leds = 0;

    for (uint8_t i = 0; i < topo->strip_count; i++)
    {
        s_total_leds += topo->strips[i].led_count;
    }

    ESP_LOGI(TAG, "Topology loaded: %d strips, %d total LEDs",
             topo->strip_count, s_total_leds);
}

uint16_t led_topology_total_leds(void)
{
    return s_total_leds;
}

uint16_t led_topology_map(uint16_t logical_index)
{
    uint16_t base = 0;

    for (uint8_t i = 0; i < s_topology->strip_count; i++)
    {
        const led_strip_t *s = &s_topology->strips[i];

        if (logical_index < base + s->led_count)
        {
            uint16_t local = logical_index - base;

            if (s->reversed)
            {
                return base + (s->led_count - 1 - local);
            }
            else
            {
                return base + local;
            }
        }

        base += s->led_count;
    }

    /* Out of bounds → clamp */
    return s_total_leds - 1;
}
