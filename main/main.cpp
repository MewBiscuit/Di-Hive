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

extern "C" {
    #include "nvs_man.h"
    #include "wifi_man.h"
    #include "mqtt_man.h"
    #include "adc_man.h"
    #include "sensors_man.h"
}

#define TAG "MAIN"
#define ENCRYPTED false
#define MAX_CHAR_SIZE 256
#define MOUNT_POINT "/SDCH"

#define SERVER "mqtt://demo.thingsboard.io"
#define TOKEN "9xtmr5zfziobg93v1ayy"
#define TOPIC "v1/devices/me/telemetry"
int port = 1883;

char ssid_var[256] = "dummy_data";
char password_var[250] = "dummy_data";
bool provisioned = false;
int max_connections = 4;
int channel = 1;

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
    }
    file.close();

    return ESP_OK;
}

esp_err_t deleteFile(fs::FS &fs, const char * path) {
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } 
    
    else {
        Serial.println("Delete failed");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

extern "C" void app_main(void) {
    //Variables
    char flag_local_data;
    int temp_in, hum_in, temp_out, noise, weight;
    char data[255];
    esp_err_t wifi_err, nvs_err, sd_err;
    time_t stamp;
    uint8_t cardType;
    esp_mqtt_client_handle_t tb_client;
    adc_oneshot_unit_handle_t adc_handle;

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
    adc_handle = adc_manager_init_oneshot(ADC_UNIT_1);
    adc_manager_cfg_channel_oneshot(adc_handle, ADC_CHANNEL_6, ADC_BITWIDTH_DEFAULT, ADC_ATTEN_DB_11);
    adc_manager_cfg_channel_oneshot(adc_handle, ADC_CHANNEL_4, ADC_BITWIDTH_DEFAULT, ADC_ATTEN_DB_11);

    //Initialize i2c sensors
    i2c_init();
    init_PmodHYGRO();
    PmodTMP3_read(&temp_out);

    //Read credentials from NVS
    nvs_err = read_string_from_nvs("ssid", ssid_var);
    if(nvs_err != ESP_OK) {
        ESP_LOGI(TAG, "No SSID found in NVS");
    }
    nvs_err = read_string_from_nvs("password", password_var);
    if(nvs_err != ESP_OK) {
        ESP_LOGI(TAG, "No password found in NVS");
    }
    nvs_err = read_string_from_nvs("flag_local_data", &flag_local_data);
    if(nvs_err != ESP_OK) {
        write_string_to_nvs("flag_local_data", "0");
        flag_local_data = '0';
    }

    //Connect to WiFi
    wifi_err = connect_ap(ssid_var, password_var);
    if (wifi_err != ESP_OK) {
        write_string_to_nvs("flag_local_data", "1");
        flag_local_data = '1';
        wifi_release();
        setup_ap("Di-Core_Prov", "provisioning112", &channel, &max_connections);
    }

    while (!is_provisioned()) {
        ESP_LOGI(TAG, "Error (%s) connecting to AP!", esp_err_to_name(wifi_err));
        for(;!provisioned;) {
            //Read sensor data
            PmodHYGRO_read(&temp_in, &hum_in);
            PmodTMP3_read(&temp_out);
            adc_manager_read_oneshot(adc_handle, ADC_CHANNEL_6, &noise);
            adc_manager_read_oneshot(adc_handle, ADC_CHANNEL_4, &weight);

            //Save data to SD card
            time(&stamp);
            snprintf(data, MAX_CHAR_SIZE, "{'ts':%lld, 'values':{'%s':%d, '%s':%d, '%s':%d, '%s':%d, '%s':%d}}", stamp, "temperature_out", temp_out,
            "temperature_in", temp_in, "humidity_in", hum_in, "noise", noise, "weight", weight);
            appendFile(SD, "/log.txt", data);

            //Can't send to sleep in order to keep prov server running
            vTaskDelay(30000 / portTICK_PERIOD_MS);
        }
    
    }

    //Connect to Thingsboard
    tb_client = connect_mqtt_token(SERVER, &port, TOKEN);
    
    if(flag_local_data == '1') {
        //TODO: Send all logged data to Thingsboard
        readFile(SD, "/log.txt");
        
        // All data sent, unmount partition and disable SDMMC peripheral
        SD.end();
        ESP_LOGI(TAG, "Card unmounted");
        
        //Turn off flag_local_data
        write_string_to_nvs("flag_local_data", "0");
    }

    // Every 30 seconds we read all of the sensors and dump the data
    PmodHYGRO_read(&temp_in, &hum_in);
    PmodTMP3_read(&temp_out);
    adc_manager_read_oneshot(adc_handle, ADC_CHANNEL_6, &noise);
    adc_manager_read_oneshot(adc_handle, ADC_CHANNEL_4, &weight);
    
    //Process data
    weight = weight - 300;
    noise = noise * 100/4095;

    //Send data to Thingsboard
    post_numerical_data("temperature_in", &temp_in, TOPIC, tb_client);
    post_numerical_data("humidity_in", &hum_in, TOPIC, tb_client);
    post_numerical_data("temperature_out", &temp_out, TOPIC, tb_client);
    post_numerical_data("noise", &noise, TOPIC, tb_client);
    post_numerical_data("weight", &weight, TOPIC, tb_client);

    vTaskDelay(3000 / portTICK_PERIOD_MS); // Wait 3 seconds to avoid sleeping before sending data
    esp_deep_sleep_start();
}