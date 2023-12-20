#include <stdio.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"

extern "C" {
    #include "nvs_man.h"
    #include "wifi_man.h"
    #include "mqtt_man.h"
    #include "adc_man.h"
    #include "sensors_man.h"
    #include "rc522.h"
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

static rc522_handle_t scanner;

static void rc522_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
    rc522_event_data_t* data = (rc522_event_data_t*) event_data;

    switch(event_id) {
        case RC522_EVENT_TAG_SCANNED: {
                rc522_tag_t* tag = (rc522_tag_t*) data->ptr;
                ESP_LOGI(TAG, "Tag scanned (sn: %" PRIu64 ")", tag->serial_number);
                //Read from NVS to check if tag is authorized
                char is_authorized[2] = "0"; // Change the declaration to an array of characters
                if(read_string_from_nvs((char*)tag->serial_number, is_authorized) == ESP_OK) { // Cast tag->serial_number to char*
                    ESP_LOGI(TAG, "Tag authorized");
                    //Open the box
                    ESP_LOGI(TAG, "Opening the box");
                }
                else {
                    ESP_LOGI(TAG, "Tag not authorized");
                }
            }
            break;
    }
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

    rc522_config_t config = {
        .spi = {
            .host = HSPI_HOST,
            .miso_gpio = 15,
            .mosi_gpio = 4,
            .sck_gpio = 16,
            .sda_gpio = 17
        }
    };

    rc522_create(&config, &scanner);
    rc522_register_events(scanner, RC522_EVENT_ANY, rc522_handler, NULL);
    rc522_start(scanner);

    //Set deep sleep to 30 secs
    esp_sleep_enable_timer_wakeup(25000000);

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
    wifi_init();
    wifi_err = connect_ap(ssid_var, password_var);
    if (wifi_err != ESP_OK) {
        write_string_to_nvs("flag_local_data", "1");
        flag_local_data = '1';
        setup_ap("Di-Core_Prov", "provisioning112", &channel, &max_connections);
    }

    while (wifi_err != ESP_OK) {
        ESP_LOGI(TAG, "Error (%s) connecting to AP!", esp_err_to_name(wifi_err));
        for(;!is_provisioned();) {
            //Read sensor data
            PmodHYGRO_read(&temp_in, &hum_in);
            PmodTMP3_read(&temp_out);
            adc_manager_read_oneshot(adc_handle, ADC_CHANNEL_6, &noise);
            adc_manager_read_oneshot(adc_handle, ADC_CHANNEL_4, &weight);

            //Save data to SD card
            time(&stamp);
            snprintf(data, MAX_CHAR_SIZE, "{'ts':%lld, 'values':{'%s':%d, '%s':%d, '%s':%d, '%s':%d, '%s':%d}}", stamp, "temperature_out", temp_out,
            "temperature_in", temp_in, "humidity_in", hum_in, "noise", noise, "weight", weight);

            //Can't send to sleep in order to keep prov server running
            vTaskDelay(30000 / portTICK_PERIOD_MS);
        }
        nvs_err = read_string_from_nvs("ssid", ssid_var);
        if(nvs_err != ESP_OK) {
            ESP_LOGI(TAG, "No SSID found in NVS");
        }
        nvs_err = read_string_from_nvs("password", password_var);
        if(nvs_err != ESP_OK) {
            ESP_LOGI(TAG, "No password found in NVS");
        }
        wifi_err = connect_ap(ssid_var, password_var);
    }

    //Connect to Thingsboard
    tb_client = connect_mqtt_token(SERVER, &port, TOKEN);
    
    if(flag_local_data == '1') {
        //TODO: Send all logged data to Thingsboard
        
        // All data sent, unmount partition and disable SDMMC peripheral
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

    vTaskDelay(5000 / portTICK_PERIOD_MS); // Wait 5 seconds to avoid sleeping before sending data and to give chance to open the box
    esp_deep_sleep_start();
}