#include <stdio.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"

#include "FS.h"
#include "SD.h"
#include "SPI.h"


//https://github.com/thingsboard/thingsboard-client-sdk/blob/master/examples/0011-esp8266_esp32_subscribe_OTA_MQTT/0011-esp8266_esp32_subscribe_OTA_MQTT.ino
#include <Espressif_Updater.h>
#include <Espressif_MQTT_Client.h>
#include <ThingsBoard.h>

extern "C" {
    #include "nvs_man.h"
    #include "wifi_man.h"
    #include "adc_man.h"
    #include "sensors_man.h"
}

#define TAG "MAIN"
#define ENCRYPTED false
#define MAX_CHAR_SIZE 256
#define MOUNT_POINT "/sdcard"

constexpr char CURRENT_FIRMWARE_TITLE[] = "Di-Hive";
constexpr char CURRENT_FIRMWARE_VERSION[] = "0.1.0";
constexpr uint8_t FIRMWARE_FAILURE_RETRIES = 12U;
constexpr uint16_t FIRMWARE_PACKET_SIZE = 4096U;
constexpr char TOKEN[] = "g4gzei5ivlqn32g824lr";
constexpr char THINGSBOARD_SERVER[] = "demo.thingsboard.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;
constexpr uint16_t MAX_FW_SIZE = FIRMWARE_PACKET_SIZE + 50U;
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;

Espressif_MQTT_Client mqtt_client;
ThingsBoard tb(mqtt_client, MAX_FW_SIZE);
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
    if (!currentFWSent) {
        currentFWSent = tb.Firmware_Send_Info(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION) && tb.Firmware_Send_State(FW_STATE_UPDATED);
    }

    if (!updateRequestSent) {
        const OTA_Update_Callback callback(&progressCallback, &updatedCallback, CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION, &updater, FIRMWARE_FAILURE_RETRIES, FIRMWARE_PACKET_SIZE);
        updateRequestSent = tb.Start_Firmware_Update(callback);
    }

    tb.loop();
}

esp_err_t appendFile(fs::FS &fs, const char * path, const char * message) {
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return ESP_FAIL;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();

    return ESP_OK;
}

esp_err_t writeFile(fs::FS &fs, const char * path, const char * message) {
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return ESP_FAIL;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
        return ESP_FAIL;
    }
    file.close();

    return ESP_OK;
}

esp_err_t readFile(fs::FS &fs, const char * path) {
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return ESP_FAIL;
    }

    Serial.print("Read from file: ");
    char buffer[255]; // Declare the buffer variable
    while(file.available()) { // Read one line at a time
        file.readStringUntil('\n').toCharArray(buffer, sizeof(buffer));
        // Process the read data here
        tb.sendTelemetryJson(buffer);
    }
    file.close();

    return ESP_OK;
}

esp_err_t deleteFile(fs::FS &fs, const char * path) {
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
        return ESP_FAIL;
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }

    return ESP_OK;
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
    char data[255];
    esp_err_t wifi_err = ESP_OK;
    esp_err_t nvs_err = ESP_OK;
    esp_err_t adc_err = ESP_OK;
    esp_err_t i2c_err = ESP_OK;
    esp_err_t sd_err = ESP_OK;
    time_t stamp;
    uint8_t cardType = NULL;

    nvs_err = init_nvs();
    if (nvs_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS! Err: %s", esp_err_to_name(nvs_err));
        return void();
    }

    //Setup SD card
    SPI.begin(GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_23, GPIO_NUM_5);
    SPI.setDataMode(SPI_MODE0);
    SD.begin(GPIO_NUM_5);
    if(!SD.begin()){
        ESP_LOGI(TAG, "Card Mount Failed");
        sd_err = ESP_FAIL;
    }

    if(sd_err != ESP_FAIL){
        ESP_LOGI(TAG, "SD card initialized");
        cardType = SD.cardType();
    }

    if(cardType == CARD_NONE){
        ESP_LOGI(TAG, "No SD card attached");
        sd_err = ESP_FAIL;
    }

    writeFile(SD, "/log.txt", "");

    //Set deep sleep to 30 secs
    esp_sleep_enable_timer_wakeup(27000000);

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
    i2c_err = i2c_init();
    i2c_err = init_PmodHYGRO();
    if (i2c_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize PmodHYGRO! Err: %s", esp_err_to_name(i2c_err));
    }
    i2c_err = PmodTMP3_read(&temp_out);
    if (i2c_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize PmodTMP3! Err: %s", esp_err_to_name(i2c_err));
    }

    //Read credentials from NVS
    nvs_err = read_string_from_nvs("ssid", ssid);
    if(nvs_err != ESP_OK) {
        ESP_LOGI(TAG, "No SSID found in NVS");
    }
    nvs_err = read_string_from_nvs("password", password);
    if(nvs_err != ESP_OK) {
        ESP_LOGI(TAG, "No password found in NVS");
    }
    nvs_err = read_string_from_nvs("token", token);
    if(nvs_err != ESP_OK) {
        //TODO: Ask for token
        ESP_LOGI(TAG, "No password found in NVS");
    }
    nvs_err = read_string_from_nvs("flag_local_data", &flag_local_data);
    if(nvs_err != ESP_OK) {
        write_string_to_nvs("flag_local_data", "0");
    }

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

        //TODO: Start provisioning server with callback for prvisioned variable
        
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

            //Save data to SD card
            time(&stamp);
            snprintf(data, MAX_CHAR_SIZE, "{'ts':%lld, 'values':{'%s':%d, '%s':%d, '%s':%d, '%s':%d, '%s':%d}}", stamp, "temperature_out", temp_out,
            "temperature_in", temp_in, "humidity_in", hum_in, "noise", noise, "weight", weight);
            appendFile(SD, "/log.txt", data);

            //Can't send to sleep to keep AP running
            vTaskDelay(30000 / portTICK_PERIOD_MS);
        }
        wifi_release();
        write_string_to_nvs("ssid", ssid);
        write_string_to_nvs("password", password);
        wifi_err = connect_ap(ssid, password);
    }
    
    if (!tb.connected()) {
        tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT);
    }

    
    if(flag_local_data == '1') {
        //Send all logged data to Thingsboard
        readFile(SD, "/log.txt");
        
        // All data sent, unmount partition and disable SDMMC peripheral
        SD.end();
        ESP_LOGI(TAG, "Card unmounted");
        
        //Turn off flag_local_data
        write_string_to_nvs("flag_local_data", "0");
    }

    //We check for OTA updates
    OTA_process();

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

        tb.sendTelemetryData("temperature_in", temp_in);
        tb.sendTelemetryData("humidity_in", hum_in);
        tb.sendTelemetryData("temperature_out", temp_out);
        tb.sendTelemetryData("noise", noise);
        tb.sendTelemetryData("weight", weight);

        tb.loop();

        vTaskDelay(3000 / portTICK_PERIOD_MS); // Wait 3 seconds to avoid sleeping before sending data

        if(!connected) {
            esp_restart();
        }

        if (!tb.connected() && attempts_recon_tb < 3) {
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
