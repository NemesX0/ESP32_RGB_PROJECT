#pragma once

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

// Mount paths - these become like "/sys/file.json"
#define FS_SYS_PATH "/sys"
#define FS_USER_PATH "/user"
#define FS_FX_PATH "/fx"

// Initialize all filesystems
esp_err_t fs_init(void);

// JSON helpers
esp_err_t fs_json_write(const char *path, const char *json);
char *fs_json_read(const char *path);

// Binary helpers (effects, webapp files, etc.)
esp_err_t fs_bin_write(const char *path, const void *data, size_t size);
uint8_t *fs_bin_read(const char *path, size_t *out_size);
