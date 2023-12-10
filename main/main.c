#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"

#include "nvs_man.h"
#include "wifi_man.h"
#include "mqtt_man.h"
#include "adc_man.h"
#include "sensors_man.h"


#define TAG "MAIN"

void app_main(void) {
    //Variables
    int temp_in = 0;
    int hum_in = 0;
    int temp_out = 0;
    int noise = 0;
    char* ssid = "";
    char* password = "";
    char* ssid_prov = "";
    char* password_prov = "";
    esp_err_t err = ESP_OK;

    //Read wifi credentials from NVS
    init_nvs();
    read_string_from_nvs("ssid", ssid);
    read_string_from_nvs("password", password);
    read_string_from_nvs("ssid_prov", ssid_prov);
    read_string_from_nvs("password_prov", password_prov);

    //Wifi setup
    wifi_init_sta();
    err = connect_ap(ssid, password);
    for(int i = 0; i < 3 && err != ESP_OK; i++) {
        ESP_LOGI(TAG, "Error (%s) connecting to AP!", esp_err_to_name(err));
        ESP_LOGI(TAG, "Error (%s) connecting to AP!", esp_err_to_name(err));
        //Start SoftAP mode
        wifi_release();
        wifi_init_ap();
        err = setup_ap(ssid_prov, password_prov, 1, 4);
        //Provide valid credentials


        //Re-attempt connection
        wifi_release();
        wifi_init_sta();
        read_string_from_nvs("ssid", ssid);
        read_string_from_nvs("password", password);
        err = connect_ap(ssid, password);
    }

    //Initialize ADC
    adc_oneshot_unit_handle_t adc_handle = adc_manager_init(ADC_UNIT_1);
    adc_manager_cfg_channel(adc_handle, ADC_CHANNEL_6, ADC_BITWIDTH_DEFAULT, ADC_ATTEN_DB_11);


    //Execution loop
    while(1) {
        //Read data
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        printf("Noise: %ddB\n", noise);
    }
}
