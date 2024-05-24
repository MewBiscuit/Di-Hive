#include "ota_man.h"

static esp_err_t validate_image_header(esp_app_desc_t *new_app_info){
    if (new_app_info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        ESP_LOGI(OTA_TAG, "Running firmware version: %s", running_app_info.version);
    }

    #ifndef CONFIG_EXAMPLE_SKIP_VERSION_CHECK
        if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0) {
            ESP_LOGW(OTA_TAG, "Current running version is the same as a new. We will not continue the update.");
            return ESP_FAIL;
        }
    #endif

    #ifdef CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
        /**
         * Secure version check from firmware image header prevents subsequent download and flash write of
         * entire firmware image. However this is optional because it is also taken care in API
         * esp_https_ota_finish at the end of OTA update procedure.
         */
        const uint32_t hw_sec_version = esp_efuse_read_secure_version();
        if (new_app_info->secure_version < hw_sec_version) {
            ESP_LOGW(OTA_TAG, "New firmware security version is less than eFuse programmed, %"PRIu32" < %"PRIu32, new_app_info->secure_version, hw_sec_version);
            return ESP_FAIL;
        }
    #endif

    return ESP_OK;
}

static esp_err_t _http_client_init_cb(esp_http_client_handle_t http_client){
    esp_err_t err = ESP_OK;
    /* Uncomment to add custom headers to HTTP request */
    // err = esp_http_client_set_header(http_client, "Custom-Header", "Value");
    return err;
}


esp_err_t check_updates() {
    esp_err_t err = ESP_OK, ota_finish_err = ESP_OK;
    esp_app_desc_t app_desc;
    esp_https_ota_handle_t https_ota_handle = NULL;

    ESP_LOGI(OTA_TAG, "Starting OTA process");

    esp_http_client_config_t config = {
        .url = CONFIG_FIRMWARE_UPGRADE_URL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = CONFIG_OTA_RECV_TIMEOUT,
        .keep_alive_enable = true,
    };

    
#ifdef CONFIG_SKIP_COMMON_NAME_CHECK
    config.skip_cert_common_name_check = true;
#endif

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
        .http_client_init_cb = _http_client_init_cb, // Register a callback to be invoked after esp_http_client is initialized
#ifdef CONFIG_ENABLE_PARTIAL_HTTP_DOWNLOAD
        .partial_http_download = true,
        .max_http_request_size = CONFIG_EXAMPLE_HTTP_REQUEST_SIZE,
#endif
    };

    err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if(err != ESP_OK) {
        ESP_LOGE(OTA_TAG, "ESP HTTPS OTA start failed");
        vTaskDelete(NULL);
        return err;
    }

    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    if(err != ESP_OK) {
        ESP_LOGE(OTA_TAG, "esp_https_ota_read_img_desc failed");
        esp_https_ota_abort(https_ota_handle);
        ESP_LOGE(OTA_TAG, "ESP_HTTPS_OTA upgrade failed");
        return err;
    }

    err = validate_image_header(&app_desc);
    if(err != ESP_OK) {
        ESP_LOGE(OTA_TAG, "image header verification failed");
        esp_https_ota_abort(https_ota_handle);
        ESP_LOGE(OTA_TAG, "ESP_HTTPS_OTA upgrade failed");
        return err;
    }

    do {
        err = esp_https_ota_perform(https_ota_handle);
        ESP_LOGD(OTA_TAG, "Image bytes read: %d", esp_https_ota_get_image_len_read(https_ota_handle));
    }while(err != ESP_ERR_HTTPS_OTA_IN_PROGRESS);

    if(esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
        // the OTA image was not completely received and user can customise the response to this situation.
        ESP_LOGE(OTA_TAG, "Complete data was not received.");
        //TODO: rollback to previous version
    } 
    
    else {
        ota_finish_err = esp_https_ota_finish(https_ota_handle);
        if((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
            ESP_LOGI(OTA_TAG, "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
            return err;
        } 
        
        else {
            if(ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
                ESP_LOGE(OTA_TAG, "Image validation failed, image is corrupted");
            }
            ESP_LOGE(OTA_TAG, "ESP_HTTPS_OTA upgrade failed 0x%x", ota_finish_err);
            return ota_finish_err;
        }
    }

    return err;
}