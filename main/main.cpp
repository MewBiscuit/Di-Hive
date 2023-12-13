#include <stdio.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"

#define THINGSBOARD_USE_ESP_PARTITION 1

#include <ThingsBoard.h>
#include <Espressif_Updater.h>
#include <Espressif_MQTT_Client.h>


extern "C" {
    #include "nvs_man.h"
    #include "wifi_man.h"
    #include "adc_man.h"
    #include "sensors_man.h"
}

#define TAG "MAIN"
#define ENCRYPTED false

constexpr char CURRENT_FIRMWARE_TITLE[] = "Di-Hive";
constexpr char CURRENT_FIRMWARE_VERSION[] = "0.1.0";

constexpr uint8_t FIRMWARE_FAILURE_RETRIES = 12U;
constexpr uint16_t FIRMWARE_PACKET_SIZE = 4096U;

constexpr char TOKEN[] = "g4gzei5ivlqn32g824lr";
constexpr char THINGSBOARD_SERVER[] = "demo.thingsboard.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;
constexpr uint16_t MAX_MESSAGE_SIZE = FIRMWARE_PACKET_SIZE + 50U;
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;

Espressif_MQTT_Client mqtt_client;
ThingsBoard tb(mqtt_client, MAX_MESSAGE_SIZE);
Espressif_Updater updater;

bool currentFWSent = false;
bool updateRequestSent = false;

void updatedCallback(const bool& success) {
  if (success) {
    ESP_LOGI(TAG, "Done, Reboot now");
    esp_restart();
    return;
  }
  ESP_LOGI(TAG, "Downloading firmware failed");
}

void progressCallback(const size_t& currentChunk, const size_t& totalChuncks) {
  ESP_LOGI("MAIN", "Downwloading firmware progress %.2f%%", static_cast<float>(currentChunk * 100U) / totalChuncks);
}

void OTA_process(){
    //OTA Update check process
    if (!currentFWSent) {
        currentFWSent = tb.Firmware_Send_Info(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION) && tb.Firmware_Send_State(FW_STATE_UPDATED);
    }

    if (!updateRequestSent) {
        const OTA_Update_Callback callback(&progressCallback, &updatedCallback, CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION, &updater, FIRMWARE_FAILURE_RETRIES, FIRMWARE_PACKET_SIZE);
        updateRequestSent = tb.Start_Firmware_Update(callback);
    }

    tb.loop();
}

extern "C" void app_main(void) {
    //Variables
    char flag_local_data = '0';
    bool provisioned = false;
    bool connected = false;
    int temp_in = 0;
    int hum_in = 0;
    int temp_out = 0;
    int noise = 0;
    int weight = 0;
    int channel = 1;
    int max_connections = 4;
    int attempts_recon_tb = 0;
    char token[] = "xxxxxxxxxxxxxxxxxxxx";
    char* ssid = "dummy_data";
    char* password = "dummy_data";
    char* ssid_prov = "Di-Core_Provisioning";
    char* password_prov = "provisioning1234!";
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

    //Read credentials from NVS
    read_string_from_nvs("ssid", ssid);
    read_string_from_nvs("password", password);
    read_string_from_nvs("token", token);
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

        for(;!provisioned;) {
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
            //TODO: Save data to SD card
            
        }
        wifi_release();
        write_string_to_nvs("ssid", ssid);
        write_string_to_nvs("password", password);
        wifi_err = connect_ap(ssid, password);
    }
    
    if (!tb.connected()) {
        tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT);
    }

    
    //TODO: If local data flag is set, dump saved data to Thingsboard with timestamp and reset flag

    OTA_process();

    //Set deep sleep to 30 secs
    esp_sleep_enable_timer_wakeup(27000000);

    //Execution loop
    for(;;) {
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

        if(!connected) {
            esp_restart();
        }

        if (!tb.connected() && attempts_recon_tb < 5) {
            tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT);
            attempts_recon_tb++;
        }
        else if (!tb.connected()) {
            ESP_LOGE(TAG, "Failed to connect to Thingsboard! Err: %s", esp_err_to_name(ESP_ERR_TIMEOUT));
            esp_restart();
        }
        else {
            attempts_recon_tb = 0;
        }
    }
}
