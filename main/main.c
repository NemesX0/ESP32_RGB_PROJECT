#include "fs.h"
#include "esp_log.h"

void app_main(void)
{
    esp_err_t err = fs_init();
    if (err == ESP_OK)
    {
        ESP_LOGI("MAIN", "Filesystem mounted successfully!");
    }
    else
    {
        ESP_LOGE("MAIN", "Filesystem mount FAILED");
    }
}
