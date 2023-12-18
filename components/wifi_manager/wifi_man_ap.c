#include "wifi_man_ap.h"
#include "wifi_man.h"

static esp_err_t wifi_init_ap() {
    esp_err_t err = ESP_OK;

    err = init_nvs();
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGI(AP_TAG, "Error (%s) initializing wifi!", esp_err_to_name(err));
    }

    return err;
}

esp_err_t start_provisioning(char *ssid, char *password) {
    esp_err_t err = ESP_OK;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_softap,
        .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE
    };
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
    ESP_LOGI(AP_TAG, "Starting provisioning");
    ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_0, NULL, ssid, password));
    return err;

}

bool is_provisioned() {
    bool provisioned = false;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
    return provisioned;
}

void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    static int retries = 0;
    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
            case WIFI_PROV_START:
                ESP_LOGI(AP_TAG, "Provisioning started");
                break;
            case WIFI_PROV_CRED_RECV: {
                wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
                ESP_LOGI(AP_TAG, "Received Wi-Fi credentials"
                         "\n\tSSID     : %s\n\tPassword : %s",
                         (const char *) wifi_sta_cfg->ssid,
                         (const char *) wifi_sta_cfg->password);
                //save ssid and password to char[] and pass to setup_ap
                char ssid[256];
                char password[256];
                memcpy(ssid, wifi_sta_cfg->ssid, strlen((const char *) wifi_sta_cfg->ssid));
                memcpy(password, wifi_sta_cfg->password, strlen((const char *) wifi_sta_cfg->password));
                write_string_to_nvs("ssid", ssid);
                write_string_to_nvs("password", password);
                wifi_release();
                connect_ap(ssid, password);
                break;
            }
            case WIFI_PROV_CRED_FAIL: {
                wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
                ESP_LOGE(AP_TAG, "Provisioning failed!\n\tReason : %s"
                         "\n\tPlease reset to factory and retry provisioning",
                         (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                         "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
                break;
            }
            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(AP_TAG, "Provisioning successful");
                retries = 0;
                break;
            case WIFI_PROV_END:
                /* De-initialize manager once provisioning is finished */
                wifi_prov_mgr_deinit();
                break;
            default:
                break;
        }
    }

    else if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(AP_TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
    }

    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(AP_TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
    }
}

esp_err_t setup_ap(char *ssid, char *password, int *channel, int *max_connections) {
    wifi_init_ap();
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));

    uint8_t len_ssid = strlen(ssid);
    uint8_t len_pass = strlen(password);

    if (len_ssid == 0 || len_ssid > 31) {
        ESP_LOGE(AP_TAG, "SSID length invalid");
        return ESP_ERR_INVALID_ARG;
    }

    if (len_pass != 0 && (len_pass < 8 || len_pass > 63)) {
        ESP_LOGE(AP_TAG, "Password length must be 8..63");
        return ESP_ERR_INVALID_ARG;
    }

    if (*channel < 1 || *channel > 13) {
        ESP_LOGE(AP_TAG, "Channel must be 1..13");
        return ESP_ERR_INVALID_ARG;
    }

    wifi_config_t wifi_config = {
        .ap =
            {
                .ssid = "",
                .ssid_len = strlen(ssid),
                .channel = *channel,
                .password = "",
                .ssid_hidden = 0,
                .max_connection = *max_connections,
                .beacon_interval = 200,
                .authmode = WIFI_AUTH_WPA2_PSK,
                .pmf_cfg =
                    {
                        .required = true,
                    },
            },
    };

    memcpy(wifi_config.ap.ssid, ssid, len_ssid);
    memcpy(wifi_config.ap.password, password, len_pass);

    if (strlen(password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(AP_TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d", ssid, password, *channel);

    start_provisioning(ssid, password);

    return ESP_OK;
}