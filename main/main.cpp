#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"

#include <Espressif_MQTT_Client.h>
#include <ThingsBoard.h>
#include <Arduino.h>

extern "C" {
    #include "nvs_man.h"
    #include "wifi_man.h"
    #include "mqtt_man.h"
    #include "adc_man.h"
    #include "sensors_man.h"
}

#define TAG "MAIN"

extern "C" void app_main(void) {
    //Variables
    char flag_local_data = '0';
    bool provisioned = false;
    int temp_in = 0;
    int hum_in = 0;
    int temp_out = 0;
    int noise = 0;
    int weight = 0;
    int channel = 1;
    int max_connections = 4;
    char* ssid = "";
    char* password = "";
    char* ssid_prov = "";
    char* password_prov = "";
    esp_err_t err = ESP_OK;

    init_nvs();

    //Initialize ADC
    adc_oneshot_unit_handle_t adc_handle = adc_manager_init_oneshot(ADC_UNIT_1);
    err = adc_manager_cfg_channel_oneshot(adc_handle, ADC_CHANNEL_6, ADC_BITWIDTH_DEFAULT, ADC_ATTEN_DB_11);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel 6! Err: %s", esp_err_to_name(err));
    }
    err = adc_manager_cfg_channel_oneshot(adc_handle, ADC_CHANNEL_4, ADC_BITWIDTH_DEFAULT, ADC_ATTEN_DB_11);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel 4! Err: %s", esp_err_to_name(err));
    }

    //Initialize other sensors
    err = init_PmodHYGRO();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize PmodHYGRO! Err: %s", esp_err_to_name(err));
    }
    err = PmodTMP3_read(&temp_out);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize PmodTMP3! Err: %s", esp_err_to_name(err));
    }

    //Read wifi credentials from NVS
    read_string_from_nvs("ssid", ssid);
    read_string_from_nvs("password", password);
    read_string_from_nvs("ssid_prov", ssid_prov);
    read_string_from_nvs("password_prov", password_prov);
    read_string_from_nvs("flag_local_data", &flag_local_data);

    //Wifi setup
    err = connect_ap(ssid, password);
    if (err != ESP_OK) { //TODO: Change errors to different types based on origin, otherwise we mix conditions between loops
        ESP_LOGI(TAG, "Error (%s) connecting to AP!", esp_err_to_name(err));

        //Turn on flag_local_data
        write_string_to_nvs("flag_local_data", "1");
        flag_local_data = '1';

        //Setup AP for provisioning
        wifi_release();
        err = setup_ap(ssid_prov, password_prov, &channel, &max_connections);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error (%s) setting up AP!", esp_err_to_name(err));
            return void();
        }

        //TODO: Start provisioning server with callback

        while(!provisioned){
            //Read sensor data
            err = PmodHYGRO_read(&temp_in, &hum_in);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to read PmodHYGRO! Err: %s", esp_err_to_name(err));
            }
            err = PmodTMP3_read(&temp_out);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to read PmodTMP3! Err: %s", esp_err_to_name(err));
            }
            err = adc_manager_read_oneshot(adc_handle, ADC_CHANNEL_6, &noise);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to read ADC channel 6! Err: %s", esp_err_to_name(err));
            }
            err = adc_manager_read_oneshot(adc_handle, ADC_CHANNEL_4, &weight);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to read ADC channel 4! Err: %s", esp_err_to_name(err));
            }

            //TODO: Save data to NVS with timestamp in order to dump the data to dashboard when connection is re-established
            

        }
        wifi_release();
        write_string_to_nvs("ssid", ssid);
        write_string_to_nvs("password", password);
        err = connect_ap(ssid, password);
    }

    //TODO: Check for OTA updates
    //TODO: Connect to Thingsboard
    //TODO: Setup partition table for OTA updates

    //TODO: Configure MQTT manager in order to save space on esp32 flash

    //Execution loop
    while(1) {
        // Every 30 seconds we read all of the sensors and dump the data
        vTaskDelay(30000 / portTICK_PERIOD_MS); //TODO: Turn into deep sleep instead of active wait
        adc_manager_read_oneshot(adc_handle, ADC_CHANNEL_6, &noise);
        adc_manager_read_oneshot(adc_handle, ADC_CHANNEL_4, &weight);
        err = PmodHYGRO_read(&temp_in, &hum_in);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read PmodHYGRO! Err: %s", esp_err_to_name(err));
        }
        err = PmodTMP3_read(&temp_out);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read PmodTMP3! Err: %s", esp_err_to_name(err));
        }
        err = adc_manager_read_oneshot(adc_handle, ADC_CHANNEL_6, &noise);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read ADC channel 6! Err: %s", esp_err_to_name(err));
        }
        err = adc_manager_read_oneshot(adc_handle, ADC_CHANNEL_4, &weight);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read ADC channel 4! Err: %s", esp_err_to_name(err));
        }
        //TODO: send data to Thingsboard
        //TODO: If disconnection is detected, we reset the esp32
    }
}
