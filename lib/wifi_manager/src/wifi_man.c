#include "wifi_man.h"

//General
void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
            case WIFI_PROV_START:
                ESP_LOGI(WIFI_TAG, "Provisioning started");
                break;
            case WIFI_PROV_CRED_RECV: {
                wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
                ESP_LOGI(WIFI_TAG, "Received Wi-Fi credentials"
                         "\n\tSSID     : %s\n\tPassword : %s",
                         (const char *) wifi_sta_cfg->ssid,
                         (const char *) wifi_sta_cfg->password);
                //save ssid and password to char[] and pass to connect_ap
                char ssid[256];
                char password[256];
                strncpy(ssid, (const char *)wifi_sta_cfg->ssid, sizeof(ssid) - 1);
                ssid[sizeof(ssid) - 1] = '\0';
                strncpy(password, (const char *)wifi_sta_cfg->password, sizeof(password) - 1);
                password[sizeof(password) - 1] = '\0';
                ESP_LOGI(WIFI_TAG, "Saving credentials to NVS");
                ESP_LOGI(WIFI_TAG, "SSID: %s", ssid);
                ESP_LOGI(WIFI_TAG, "Password: %s", password);
                write_string_to_nvs("ssid", ssid);
                write_string_to_nvs("password", password);
                break;
            }
            case WIFI_PROV_CRED_FAIL: {
                wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
                ESP_LOGE(WIFI_TAG, "Provisioning failed!\n\tReason : %s"
                         "\n\tPlease reset to factory and retry provisioning",
                         (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                         "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
                wifi_prov_mgr_reset_sm_state_on_failure();
                break;
            }
            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(WIFI_TAG, "Provisioning successful");
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
        ESP_LOGI(WIFI_TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
    }

    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(WIFI_TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
    }

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
        esp_wifi_connect();

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(WIFI_TAG, "Retrying to connect to AP");
        }

        else
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);

        ESP_LOGI(WIFI_TAG, "Connection to the AP fail");
    }

    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(WIFI_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_init() {
    esp_err_t err = ESP_OK;
    int i;

    for (i = 0; i < 10 && err != ESP_OK; i++){
        err = init_nvs();
    }

    if(err != ESP_OK){
        ESP_LOGE(WIFI_TAG, "Couldn't fix error initializing NVS");
        return err;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGI(WIFI_TAG, "Error %d initializing wifi!", err);
    }

    return err;
}

esp_err_t wifi_release() {
    esp_err_t err = ESP_OK;
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_event_loop_delete_default());
    return err;
}

//Wifi STA
esp_err_t connect_ap(const char *ssid, const char *password) {
    esp_err_t err = ESP_OK;
    s_wifi_event_group = xEventGroupCreate();
    EventBits_t bits;
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    int len_ssid = strlen(ssid), len_pass = strlen(password);

    err = esp_netif_init();
    if(err != ESP_OK) {
        ESP_LOGE(WIFI_TAG, "Error initializing TCP/IP stack: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id);
    if(err != ESP_OK) {
        ESP_LOGE(WIFI_TAG, "Error registering event handler for WIFI_EVENT: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip);
    if(err != ESP_OK) {
        ESP_LOGE(WIFI_TAG, "Error registering event handler for IP_EVENT: %s", esp_err_to_name(err));
        return err;
    }

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

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if(err != ESP_OK) {
        ESP_LOGE(WIFI_TAG, "Error setting wifi mode as STA: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if(err != ESP_OK) {
        ESP_LOGE(WIFI_TAG, "Error setting config of STA: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_wifi_start();
    if(err != ESP_OK) {
        ESP_LOGE(WIFI_TAG, "Error starting WiFi: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(WIFI_TAG, "Finished setting up ESP WiFi module as Station");

    bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if(bits & WIFI_CONNECTED_BIT)
        ESP_LOGI(WIFI_TAG, "Connected to AP with SSID:%s", ssid);

    else if(bits & WIFI_FAIL_BIT) {
        ESP_LOGI(WIFI_TAG, "Failed to connect to SSID:%s", ssid);
        return ESP_FAIL;
    }

    else {
        ESP_LOGE(WIFI_TAG, "UNEXPECTED EVENT");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t disconnect_ap() {
    esp_err_t err = ESP_OK;
    err = esp_wifi_disconnect();
    if (err != ESP_OK) {
        ESP_LOGI(WIFI_TAG, "Error (%s) disconnecting from ap!", esp_err_to_name(err));
    }
    return err;
}

//Wifi AP
esp_err_t custom_prov_data_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen, uint8_t **outbuf, ssize_t *outlen, void *priv_data) {
    char response[] = "SUCCESS";
    
    if(inbuf) {
        ESP_LOGI(WIFI_TAG, "Received data: %.*s", inlen, (char *)inbuf);
    }

    *outbuf = (uint8_t *)strdup(response);

    if(*outbuf == NULL) {
        ESP_LOGE(WIFI_TAG, "System out of memory");
        return ESP_ERR_NO_MEM;
    }

    *outlen = strlen(response) + 1; /* +1 for NULL terminating byte */

    return ESP_OK;
}


esp_err_t start_provisioning(char *ssid, char *password) {
    esp_err_t err = ESP_OK;

    err = esp_event_handler_instance_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL);
    if(err != ESP_OK) {
        ESP_LOGE(WIFI_TAG, "Error registering event handler for WIFI_PROV_EVENT: %s", esp_err_to_name(err));
        return err;
    }

    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_softap,
        .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE
    };

    err = wifi_prov_mgr_init(config);
    if(err != ESP_OK) {
        ESP_LOGE(WIFI_TAG, "Error initializing provisioning manager: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(WIFI_TAG, "Starting provisioning scheme");

    err = wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_0, NULL, ssid, password);
    if(err != ESP_OK) {
        ESP_LOGE(WIFI_TAG, "Error setting starting provisioning service: %s", esp_err_to_name(err));
        return err;
    }

    err = wifi_prov_mgr_endpoint_register("custom-data", custom_prov_data_handler, NULL);
    if(err != ESP_OK) {
        ESP_LOGE(WIFI_TAG, "Error registering handler for provisioning endpoing: %s", esp_err_to_name(err));
    }
    return err;
}

bool is_provisioned(bool* provisioned) {
    esp_err_t err = ESP_OK;
    
    err = wifi_prov_mgr_is_provisioned(provisioned);
    if(err == ESP_FAIL) {
        ESP_LOGE(WIFI_TAG, "Error checking if device is provisioned: %d", err);
    }

    return err;
}

esp_err_t setup_ap(char *ssid, char *password, int *channel, int *max_connections) {
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));

    uint8_t len_ssid = strlen(ssid);
    uint8_t len_pass = strlen(password);

    if (len_ssid == 0 || len_ssid > 31) {
        ESP_LOGE(WIFI_TAG, "SSID length invalid");
        return ESP_ERR_INVALID_ARG;
    }

    if (len_pass != 0 && (len_pass < 8 || len_pass > 63)) {
        ESP_LOGE(WIFI_TAG, "Password length must be 8..63");
        return ESP_ERR_INVALID_ARG;
    }

    if (*channel < 1 || *channel > 13) {
        ESP_LOGE(WIFI_TAG, "Channel must be 1..13");
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

    ESP_LOGI(WIFI_TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d", ssid, password, *channel);

    start_provisioning(ssid, password);

    return ESP_OK;
}