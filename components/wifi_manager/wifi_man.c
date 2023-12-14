#include "wifi_man.h"

esp_err_t wifi_release() {
    esp_err_t err = ESP_OK;
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_event_loop_delete_default());
    return err;
}