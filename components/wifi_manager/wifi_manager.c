#include "wifi_manager.h"

#include <stdint.h>
#include "esp_wifi.h"
#include "esp_check.h"
#include "nvs_flash.h"

static const char* TAG = "wifi_manager";

esp_err_t wifi_init()
{
    esp_err_t err = ESP_OK;

    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
    }

    err = esp_wifi_init(WIFI_INIT_CONFIG_DEFAULT());
    if(err != ESP_OK)
    {
        ESP_LOGI(TAG, "Error (%s) initializing wifi!", esp_err_to_name(err));
    }

    ESP_ERROR_CHECK(err);

    return err;
}

esp_err_t wifi_release()
{
    esp_err_t err = ESP_OK;
    err = esp_wifi_deinit();
    if(err != ESP_OK)
    {
        ESP_LOGI(TAG, "Error (%s) releasing wifi!", esp_err_to_name(err));
    }
    return err;
}