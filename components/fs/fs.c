#include "fs.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "fs";

// ---------- LittleFS MOUNT HELPER ----------
static esp_err_t mount_lfs(const char *label, const char *path)
{
    const esp_vfs_littlefs_conf_t conf = {
        .base_path = path,
        .partition_label = label,
        .format_if_mount_failed = true,
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to mount partition '%s': %s",
                 label, esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Mounted FS: %s at %s", label, path);
    return ESP_OK;
}

esp_err_t fs_init(void)
{
    ESP_ERROR_CHECK(mount_lfs("sysdata", FS_SYS_PATH));
    ESP_ERROR_CHECK(mount_lfs("userdata", FS_USER_PATH));
    ESP_ERROR_CHECK(mount_lfs("effects", FS_FX_PATH));

    return ESP_OK;
}

// ---------- JSON READ/WRITE ----------
esp_err_t fs_json_write(const char *path, const char *json)
{
    FILE *f = fopen(path, "w");
    if (!f)
        return ESP_FAIL;

    fwrite(json, 1, strlen(json), f);
    fclose(f);
    return ESP_OK;
}

char *fs_json_read(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buf = malloc(size + 1);
    fread(buf, 1, size, f);
    buf[size] = '\0';

    fclose(f);
    return buf;
}

// ---------- BINARY READ/WRITE ----------
esp_err_t fs_bin_write(const char *path, const void *data, size_t size)
{
    FILE *f = fopen(path, "wb");
    if (!f)
        return ESP_FAIL;

    fwrite(data, 1, size, f);
    fclose(f);
    return ESP_OK;
}

uint8_t *fs_bin_read(const char *path, size_t *out_size)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    uint8_t *buffer = malloc(size);
    fread(buffer, 1, size, f);
    fclose(f);

    if (out_size)
        *out_size = size;
    return buffer;
}
