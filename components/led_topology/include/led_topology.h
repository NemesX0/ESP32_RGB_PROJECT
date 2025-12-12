#pragma once

#include <stdint.h>
#include <stdbool.h>

/* One LED strip definition */
typedef struct
{
    uint16_t led_count;
    bool reversed; // true = physical strip is wired backwards
} led_strip_t;

/* Full bike LED layout */
typedef struct
{
    uint8_t strip_count;
    const led_strip_t *strips;
} led_topology_t;

/* Initialize topology */
void led_topology_init(const led_topology_t *topo);

/* Total LEDs across all strips */
uint16_t led_topology_total_leds(void);

/* Map logical index → physical WS2812 buffer index */
uint16_t led_topology_map(uint16_t logical_index);
