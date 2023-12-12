#include <stdio.h>
#include <string.h>
#include <time.h>

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

    //TODO: Setup partition table for OTA updates

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
    char* timestamp = __TIMESTAMP__;
    esp_err_t wifi_err = ESP_OK;
    esp_err_t adc_err = ESP_OK;
    esp_err_t i2c_err = ESP_OK;


    init_nvs();

    //Initialize ADC
    adc_oneshot_unit_handle_t adc_handle = adc_manager_init_oneshot(ADC_UNIT_1);
    adc_err = adc_manager_cfg_channel_oneshot(adc_handle, ADC_CHANNEL_6, ADC_BITWIDTH_DEFAULT, ADC_ATTEN_DB_11);
    if (adc_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel 6! Err: %s", esp_err_to_name(adc_err));
    }
    adc_err = adc_manager_cfg_channel_oneshot(adc_handle, ADC_CHANNEL_4, ADC_BITWIDTH_DEFAULT, ADC_ATTEN_DB_11);
    if (adc_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel 4! Err: %s", esp_err_to_name(adc_err));
    }

    //Initialize other sensors
    i2c_err = init_PmodHYGRO();
    if (i2c_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize PmodHYGRO! Err: %s", esp_err_to_name(i2c_err));
    }
    i2c_err = PmodTMP3_read(&temp_out);
    if (i2c_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize PmodTMP3! Err: %s", esp_err_to_name(i2c_err));
    }

    //Read wifi credentials from NVS
    read_string_from_nvs("ssid", ssid);
    read_string_from_nvs("password", password);
    read_string_from_nvs("ssid_prov", ssid_prov);
    read_string_from_nvs("password_prov", password_prov);
    read_string_from_nvs("flag_local_data", &flag_local_data);

    //Wifi setup
    wifi_err = connect_ap(ssid, password);
    if (wifi_err != ESP_OK) {
        ESP_LOGI(TAG, "Error (%s) connecting to AP!", esp_err_to_name(wifi_err));

        //Turn on flag_local_data
        write_string_to_nvs("flag_local_data", "1");
        flag_local_data = '1';

        //Setup AP for provisioning
        wifi_release();
        wifi_err = setup_ap(ssid_prov, password_prov, &channel, &max_connections);
        if (wifi_err != ESP_OK) {
            ESP_LOGE(TAG, "Error (%s) setting up AP!", esp_err_to_name(wifi_err));
            return void();
        }

        //TODO: Start provisioning server with callback

        while(!provisioned){
            //Read sensor data
            i2c_err = PmodHYGRO_read(&temp_in, &hum_in);
            if (i2c_err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to read PmodHYGRO! Err: %s", esp_err_to_name(i2c_err));
            }
            i2c_err = PmodTMP3_read(&temp_out);
            if (i2c_err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to read PmodTMP3! Err: %s", esp_err_to_name(i2c_err));
            }
            adc_err = adc_manager_read_oneshot(adc_handle, ADC_CHANNEL_6, &noise);
            if (adc_err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to read ADC channel 6! Err: %s", esp_err_to_name(adc_err));
            }adc_err = adc_manager_read_oneshot(adc_handle, ADC_CHANNEL_4, &weight);
            if (adc_err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to read ADC channel 4! Err: %s", esp_err_to_name(adc_err));
            }
            timestamp = __TIMESTAMP__;
            //TODO: Migrate to SD card
            //Saving data to nvs
            write_string_to_nvs("temp_in " + timestamp, std::to_string(temp_in).c_str());
            write_string_to_nvs("hum_in " + timestamp, std::to_string(hum_in).c_str());
            write_string_to_nvs("temp_out " + timestamp, std::to_string(temp_out).c_str());
            write_string_to_nvs("noise " + timestamp, std::to_string(noise).c_str());
            write_string_to_nvs("weight " + timestamp, std::to_string(weight).c_str());
            

        }
        wifi_release();
        write_string_to_nvs("ssid", ssid);
        write_string_to_nvs("password", password);
        wifi_err = connect_ap(ssid, password);
    }
    
    //TODO: Connect to Thingsboard

    //TODO: Check for OTA updates

    //TODO: If local data flag is set, dump saved data to Thingsboard with timestamp and reset flag

    //Set deep sleep to 30 secs
    esp_sleep_enable_timer_wakeup(27000000);

    //Execution loop
    while(1) {
        // Every 30 seconds we read all of the sensors and dump the data
        esp_deep_sleep_start();
        i2c_err = PmodHYGRO_read(&temp_in, &hum_in);
        if (i2c_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read PmodHYGRO! Err: %s", esp_err_to_name(i2c_err));
        }
        i2c_err = PmodTMP3_read(&temp_out);
        if (i2c_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read PmodTMP3! Err: %s", esp_err_to_name(i2c_err));
        }
        adc_err = adc_manager_read_oneshot(adc_handle, ADC_CHANNEL_6, &noise);
        if (adc_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read ADC channel 6! Err: %s", esp_err_to_name(adc_err));
        }
        adc_err = adc_manager_read_oneshot(adc_handle, ADC_CHANNEL_4, &weight);
        if (adc_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read ADC channel 4! Err: %s", esp_err_to_name(adc_err));
        }
        //TODO: send data to Thingsboard

        vTaskDelay(3000 / portTICK_PERIOD_MS); // Wait 3 seconds to avoid sleeping before sending data

        //TODO: If disconnection is detected, we reset the esp32
        
    }
}
