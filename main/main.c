//Standard libraries
#include <stdio.h>
#include <string.h>
#include <time.h>

//ESP specific libraries
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"

//Homebrewed libraries
#include "nvs_man.h"
#include "wifi_man.h"
#include "mqtt_man.h"
#include "adc_man.h"
#include "sensors_man.h"

//Logs
#define TAG "MAIN"

//Connectivity
#define SERVER "mqtt://demo.thingsboard.io"
#define TOKEN ""
#define TOPIC "v1/devices/me/telemetry"
int port = 1883;

//NVS
char ssid_var[256] = "dummy_data";
char password_var[250] = "dummy_data";

void app_main() {
    int i;
    float temp, hum;
    i2c_master_bus_handle_t i2c_bus;
    I2C_Sensor sht40;
    sht40.write_data = 0xFD;

    i2c_init(&i2c_bus);

    sht40.i2c_bus_handle = i2c_bus;

    SHT40_init(&sht40);

    for(; 1;){
        SHT40_read(&sht40, &temp, &hum);
        printf("Temp: %f Cº       Hum: %f Percent \n", temp, hum);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}