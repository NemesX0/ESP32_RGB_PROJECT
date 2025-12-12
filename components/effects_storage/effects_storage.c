#include "effects_storage.h"
#include "fs.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#define TAG "effects"

#define EFFECT_MAGIC 0x4D525847 // "MRXG"
#define EFFECT_VERSION 1

struct effect_handle
{
    effect_info_t info;
    uint8_t *frames; // raw RGB data
};

typedef struct __attribute__((packed))
{
    uint32_t magic;
    uint16_t version;
    uint16_t frame_count;
    uint16_t leds_per_frame;
    uint16_t frame_delay_ms;
} effect_header_t;

esp_err_t effects_init(void)
{
    // Ensure effects partition is mounted
    return fs_ensure_dir(EFFECTS_BASE_PATH);
}

esp_err_t effects_list(char ***out_names, size_t *out_count)
{
    DIR *dir = opendir(EFFECTS_BASE_PATH);
    if (!dir)
        return ESP_ERR_NOT_FOUND;

    struct dirent *ent;
    size_t count = 0;

    while ((ent = readdir(dir)))
    {
        if (ent->d_type == DT_REG)
            count++;
    }
    rewinddir(dir);

    char **names = calloc(count, sizeof(char *));
    if (!names)
    {
        closedir(dir);
        return ESP_ERR_NO_MEM;
    }

    size_t idx = 0;
    while ((ent = readdir(dir)))
    {
        if (ent->d_type != DT_REG)
            continue;
        names[idx++] = strdup(ent->d_name);
    }

    closedir(dir);
    *out_names = names;
    *out_count = count;
    return ESP_OK;
}

void effects_list_free(char **names, size_t count)
{
    for (size_t i = 0; i < count; i++)
        free(names[i]);
    free(names);
}

esp_err_t effects_open(const char *name, effect_handle_t **out)
{
    char path[128];
    snprintf(path, sizeof(path), "%s/%s", EFFECTS_BASE_PATH, name);

    FILE *f = fopen(path, "rb");
    if (!f)
        return ESP_ERR_NOT_FOUND;

    effect_header_t hdr;
    if (fread(&hdr, sizeof(hdr), 1, f) != 1)
    {
        fclose(f);
        return ESP_FAIL;
    }

    if (hdr.magic != EFFECT_MAGIC || hdr.version != EFFECT_VERSION)
    {
        fclose(f);
        return ESP_ERR_INVALID_VERSION;
    }

    size_t frame_size = hdr.leds_per_frame * 3;
    size_t total_size = frame_size * hdr.frame_count;

    uint8_t *frames = malloc(total_size);
    if (!frames)
    {
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    if (fread(frames, 1, total_size, f) != total_size)
    {
        free(frames);
        fclose(f);
        return ESP_FAIL;
    }

    fclose(f);

    effect_handle_t *h = calloc(1, sizeof(*h));
    if (!h)
    {
        free(frames);
        return ESP_ERR_NO_MEM;
    }

    h->info.frame_count = hdr.frame_count;
    h->info.leds_per_frame = hdr.leds_per_frame;
    h->info.frame_delay_ms = hdr.frame_delay_ms;
    h->frames = frames;

    *out = h;

    ESP_LOGI(TAG, "Loaded effect '%s' (%u frames, %u leds)",
             name, hdr.frame_count, hdr.leds_per_frame);

    return ESP_OK;
}

void effects_close(effect_handle_t *effect)
{
    if (!effect)
        return;
    free(effect->frames);
    free(effect);
}

const effect_info_t *effects_get_info(effect_handle_t *effect)
{
    return effect ? &effect->info : NULL;
}

const uint8_t *effects_get_frame(effect_handle_t *effect, uint16_t frame_index)
{
    if (!effect || frame_index >= effect->info.frame_count)
        return NULL;

    size_t frame_size = effect->info.leds_per_frame * 3;
    return effect->frames + (frame_index * frame_size);
}
