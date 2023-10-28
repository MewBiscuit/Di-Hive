#include "wifi_manager.h"

#include <stdint.h>
#include "esp_wifi.h"
#include "esp_check.h"

static const char* TAG = "wifi_manager";

esp_err_t wifi_init()
{
    esp_err_t err = ESP_OK;
    err = esp_wifi_init(WIFI_INIT_CONFIG_DEFAULT);
    if(err != ESP_OK)
    {
        ESP_LOGI(TAG, "Error (%s) initializing wifi!", esp_err_to_name(err));
    }

    return err;
}

esp_err_t wifi_setup_ap(const char* ssid, const char* password)
{
    esp_err_t err = ESP_OK;
    
    // Set wifi mode to access point
    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if(err != ESP_OK)
    {
        ESP_LOGI(TAG, "Error (%s) setting wifi mode!", esp_err_to_name(err));
        return err;
    }

    // Set wifi configuration
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ssid,
            .ssid_len = strlen(ssid),
            .password = password,
            .max_connection = 10,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        }
    };
    err = esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config);

    // Start ap with specified configuration
    err = esp_wifi_start();
    if(err != ESP_OK)
    {
        ESP_LOGI(TAG, "Error (%s) starting wifi!", esp_err_to_name(err));
        return err;
    }

    return err;
}

esp_err_t wifi_connect_ap(const char* ssid, const char* password)
{
    esp_err_t err = ESP_OK;
    
    // Set wifi mode to station
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if(err != ESP_OK)
    {
        ESP_LOGI(TAG, "Error (%s) setting wifi mode!", esp_err_to_name(err));
        return err;
    }

    // Set wifi configuration
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ssid,
            .password = password,
            .bssid_set = false
        }
    };
    err = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);

    // Start ap with specified configuration
    err = esp_wifi_start();
    if(err != ESP_OK)
    {
        ESP_LOGI(TAG, "Error (%s) starting wifi!", esp_err_to_name(err));
        return err;
    }

    // Connect to specified ap
    err = esp_wifi_connect();
    if(err != ESP_OK)
    {
        ESP_LOGI(TAG, "Error (%s) connecting to ap!", esp_err_to_name(err));

    }

    return err;
}

esp_err_t disconnect_ap()
{
    esp_err_t err = ESP_OK;
    err = esp_wifi_disconnect();
    if(err != ESP_OK)
    {
        ESP_LOGI(TAG, "Error (%s) disconnecting from ap!", esp_err_to_name(err));
    }
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