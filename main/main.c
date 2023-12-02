#include <stdio.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"

#include "mqtt_man.h"
#include "wifi_man.h"
#include "adc_man.h"


void app_main(void)
{
    //Variables
    int data = 0;

    //Connect to wifi
    connect_ap("IA", "hola1112");

    //Enable sleep rtc wakeup
    const int wakeup_time_sec = 10;
    printf("Enabling timer wakeup, %ds\n", wakeup_time_sec);
    esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000);
    adc_oneshot_unit_handle_t adc_handle = adc_manager_init(ADC_UNIT_1);
    adc_manager_cfg_channel(adc_handle, ADC_CHANNEL_4, ADC_BITWIDTH_DEFAULT, ADC_ATTEN_DB_11);


    //Execution loop
    while(1){
        //Read data
        adc_manager_read_oneshot(adc_handle, ADC_CHANNEL_4, &data);
        printf("ADC value: %d\n", data);
        esp_deep_sleep_start();
    }
}
