#include <stdio.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"

#include "adc_man.h"
#include "sensors_man.h"
#include "nvs_man.h"
#include "wifi_man.h"
#include "mqtt_man.h"


#define TAG "MAIN"

static esp_err_t groove_mic(int* noise, adc_oneshot_unit_handle_t adc_handle, adc_channel_t channel){
    esp_err_t ret = ESP_OK;
    ret = adc_manager_read_oneshot(adc_handle, channel, noise);
    return ret;
}

void app_main(void)
{
    //Variables
    int temp_in = 0;
    int hum_in = 0;
    int temp_out = 0;
    int noise = 0;

    //Initialize ADC
    adc_oneshot_unit_handle_t adc_handle = adc_manager_init(ADC_UNIT_1);
    adc_manager_cfg_channel(adc_handle, ADC_CHANNEL_6, ADC_BITWIDTH_DEFAULT, ADC_ATTEN_DB_11);


    //Execution loop
    while(1){
        //Read data
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        groove_mic(&noise, adc_handle, ADC_CHANNEL_6);
        printf("Noise: %ddB\n", noise);
    }
}
