#pragma once

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#define EFFECTS_BASE_PATH "/fx"

typedef struct
{
    uint16_t frame_count;
    uint16_t leds_per_frame; // virtual LEDs
    uint16_t frame_delay_ms;
} effect_info_t;

typedef struct effect_handle effect_handle_t;

// lifecycle
esp_err_t effects_init(void);

// discovery
esp_err_t effects_list(char ***out_names, size_t *out_count);
void effects_list_free(char **names, size_t count);

// loading
esp_err_t effects_open(const char *name, effect_handle_t **out);
void effects_close(effect_handle_t *effect);

// metadata
const effect_info_t *effects_get_info(effect_handle_t *effect);

// frame access
const uint8_t *effects_get_frame(effect_handle_t *effect, uint16_t frame_index);
