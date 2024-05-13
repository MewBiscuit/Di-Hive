#include "nvs_man.h"

esp_err_t init_nvs() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    return err;
}

esp_err_t read_string_from_nvs(char *key, char *value) {
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(NVS_TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    } 

    else {
        // Read
        size_t required_size = 0;  // value will default to NULL if not set yet in NVS
        err = nvs_get_str(my_handle, key, NULL, &required_size);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGE(NVS_TAG, "Error (%s) reading!", esp_err_to_name(err));
        } 

        else {
            if (required_size > 0) {
                err = nvs_get_str(my_handle, key, value, &required_size);
                if (err != ESP_OK) {
                    ESP_LOGE(NVS_TAG, "Error (%s) reading!", esp_err_to_name(err));
                }
            } 
            
            else {
                ESP_LOGI(NVS_TAG, "The value is not initialized yet!");
            }
        }
    }
    // Close
    nvs_close(my_handle);
    return err;
}

esp_err_t write_string_to_nvs(char* key, char* value) {
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(NVS_TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    } 
    
    else {
        // Write
        err = nvs_set_str(my_handle, key, value);
        if (err != ESP_OK) {
            ESP_LOGE(NVS_TAG, "Error (%s) writing!", esp_err_to_name(err));
        }

        // Commit
        err = nvs_commit(my_handle);
        if (err != ESP_OK) {
            ESP_LOGE(NVS_TAG, "Error (%s) committing!", esp_err_to_name(err));
        }
    }
    // Close
    nvs_close(my_handle);
    return err;
}