#include "wifi_man_sta.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define ESP_MAXIMUM_RETRY 10

static int s_retry_num = 0;

static esp_err_t wifi_init_sta() {
    esp_err_t err = ESP_OK;

    err = init_nvs();
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGI(STA_TAG, "Error (%s) initializing wifi!", esp_err_to_name(err));
    }

    return err;
}

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
        esp_wifi_connect();

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(STA_TAG, "Retrying to connect to AP");
        }

        else
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);

        ESP_LOGI(STA_TAG, "Connection to the AP fail");
    }

    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(STA_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t connect_ap(const char *ssid, const char *password) {
    wifi_init_sta();
    s_wifi_event_group = xEventGroupCreate();
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    int len_ssid = strlen(ssid);
    int len_pass = strlen(password);

    wifi_config_t wifi_config = {
        .sta =
            {
                .ssid = "",
                .password = "",
                .threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK,
                .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            },
    };

    memcpy(wifi_config.ap.ssid, ssid, len_ssid);
    memcpy(wifi_config.ap.password, password, len_pass);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(STA_TAG, "Finished setting up ESP as wifi station");

    EventBits_t bits =
        xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT)
        ESP_LOGI(STA_TAG, "Connected to ap with SSID:%s password:%s", ssid, password);

    else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(STA_TAG, "Failed to connect to SSID:%s, password:%s", ssid, password);
        return ESP_FAIL;
    }

    else {
        ESP_LOGE(STA_TAG, "UNEXPECTED EVENT");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t disconnect_ap() {
    esp_err_t err = ESP_OK;
    err = esp_wifi_disconnect();
    if (err != ESP_OK) {
        ESP_LOGI(STA_TAG, "Error (%s) disconnecting from ap!", esp_err_to_name(err));
    }
    return err;
}