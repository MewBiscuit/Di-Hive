#include "wifi_manager.h"

#include <stdint.h>

#include "esp_check.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

static const char *TAG = "wifi_manager";

esp_err_t wifi_release()
{
    esp_err_t err = ESP_OK;
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_event_loop_delete_default());
    ESP_ERROR_CHECK(esp_netif_deinit());
    return err;
}