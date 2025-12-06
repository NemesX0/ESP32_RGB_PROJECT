#include "config_system.h"
#include "fs.h"

#include "cJSON.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "config_system";

#define SYSTEM_CONFIG_PATH FS_SYS_PATH "/system.json"

static system_config_t g_cfg;
static bool g_loaded = false;

// ---------- helpers ----------

static void set_defaults(system_config_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));

    cfg->provisioned = false;
    cfg->led_gpio = 48; // default placeholder, user changes later
    cfg->led_count = 1; // default placeholder

    strncpy(cfg->device_name, "MotoRGB", sizeof(cfg->device_name) - 1);
    strncpy(cfg->ble_name, "MotoRGB", sizeof(cfg->ble_name) - 1);
    strncpy(cfg->ap_password, "moto1234", sizeof(cfg->ap_password) - 1);
    strncpy(cfg->ble_pin, "123456", sizeof(cfg->ble_pin) - 1);
}

static esp_err_t from_json(const char *json, system_config_t *out)
{
    cJSON *root = cJSON_Parse(json);
    if (!root)
    {
        ESP_LOGE(TAG, "Failed to parse system.json");
        return ESP_FAIL;
    }

    set_defaults(out); // start from defaults, then override

    cJSON *j_prov = cJSON_GetObjectItemCaseSensitive(root, "provisioned");
    if (cJSON_IsBool(j_prov))
    {
        out->provisioned = cJSON_IsTrue(j_prov);
    }

    cJSON *j_gpio = cJSON_GetObjectItemCaseSensitive(root, "led_gpio");
    if (cJSON_IsNumber(j_gpio))
    {
        out->led_gpio = j_gpio->valueint;
    }

    cJSON *j_count = cJSON_GetObjectItemCaseSensitive(root, "led_count");
    if (cJSON_IsNumber(j_count))
    {
        out->led_count = (uint32_t)j_count->valuedouble;
    }

    cJSON *j_devname = cJSON_GetObjectItemCaseSensitive(root, "device_name");
    if (cJSON_IsString(j_devname) && j_devname->valuestring)
    {
        strncpy(out->device_name, j_devname->valuestring,
                sizeof(out->device_name) - 1);
    }

    cJSON *j_blename = cJSON_GetObjectItemCaseSensitive(root, "ble_name");
    if (cJSON_IsString(j_blename) && j_blename->valuestring)
    {
        strncpy(out->ble_name, j_blename->valuestring,
                sizeof(out->ble_name) - 1);
    }

    cJSON *j_ap = cJSON_GetObjectItemCaseSensitive(root, "ap_password");
    if (cJSON_IsString(j_ap) && j_ap->valuestring)
    {
        strncpy(out->ap_password, j_ap->valuestring,
                sizeof(out->ap_password) - 1);
    }

    cJSON *j_pin = cJSON_GetObjectItemCaseSensitive(root, "ble_pin");
    if (cJSON_IsString(j_pin) && j_pin->valuestring)
    {
        strncpy(out->ble_pin, j_pin->valuestring,
                sizeof(out->ble_pin) - 1);
    }

    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t to_json(const system_config_t *cfg, char **out_json)
{
    cJSON *root = cJSON_CreateObject();
    if (!root)
        return ESP_ERR_NO_MEM;

    cJSON_AddBoolToObject(root, "provisioned", cfg->provisioned);
    cJSON_AddNumberToObject(root, "led_gpio", cfg->led_gpio);
    cJSON_AddNumberToObject(root, "led_count", (double)cfg->led_count);
    cJSON_AddStringToObject(root, "device_name", cfg->device_name);
    cJSON_AddStringToObject(root, "ble_name", cfg->ble_name);
    cJSON_AddStringToObject(root, "ap_password", cfg->ap_password);
    cJSON_AddStringToObject(root, "ble_pin", cfg->ble_pin);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (!json)
        return ESP_ERR_NO_MEM;

    *out_json = json;
    return ESP_OK;
}

// ---------- public API ----------

esp_err_t system_config_init(void)
{
    if (g_loaded)
        return ESP_OK;

    char *json = fs_json_read(SYSTEM_CONFIG_PATH);
    if (!json)
    {
        ESP_LOGW(TAG, "system.json not found, creating defaults");
        set_defaults(&g_cfg);
        g_loaded = true;
        return system_config_save();
    }

    esp_err_t err = from_json(json, &g_cfg);
    free(json);

    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Invalid system.json, resetting to defaults");
        set_defaults(&g_cfg);
        g_loaded = true;
        return system_config_save();
    }

    g_loaded = true;
    ESP_LOGI(TAG, "System config loaded (provisioned=%d, gpio=%d, leds=%lu)",
             g_cfg.provisioned, g_cfg.led_gpio, (unsigned long)g_cfg.led_count);
    return ESP_OK;
}

const system_config_t *system_config_get(void)
{
    return &g_cfg;
}

esp_err_t system_config_save(void)
{
    char *json = NULL;
    esp_err_t err = to_json(&g_cfg, &json);
    if (err != ESP_OK)
        return err;

    err = fs_json_write(SYSTEM_CONFIG_PATH, json);
    free(json);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "System config saved");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to save system config");
    }

    return err;
}

esp_err_t system_config_set_provisioned(bool value)
{
    g_cfg.provisioned = value;
    return system_config_save();
}

esp_err_t system_config_factory_reset(void)
{
    set_defaults(&g_cfg);
    return system_config_save();
}

bool system_config_is_provisioned(void)
{
    return g_cfg.provisioned;
}

int system_config_get_led_gpio(void)
{
    return g_cfg.led_gpio;
}

uint32_t system_config_get_led_count(void)
{
    return g_cfg.led_count;
}

const char *system_config_get_ble_name(void)
{
    return g_cfg.ble_name;
}

const char *system_config_get_device_name(void)
{
    return g_cfg.device_name;
}
