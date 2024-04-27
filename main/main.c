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

//External libraries
#include "ssd1306.h"
#include "font8x8_basic.h"
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
    SSD1306_t ssd1306;
    I2C_Sensor sht40;
    sht40.write_data = 0xFD;
    char hum_display[16], temp_display[16];

    i2c_bus_init(&i2c_bus, 1, 27, 14);
    i2c_master_init(&ssd1306, 32, 33, CONFIG_RESET_GPIO);

	ssd1306_init(&ssd1306, 128, 64);
    ssd1306_contrast(&ssd1306, 0xff);
    ssd1306_clear_screen(&ssd1306, false);

    sht40.i2c_bus_handle = &i2c_bus;

    SHT40_init(&sht40);

    for(; 1;){
        SHT40_read(&sht40, &temp, &hum);
        printf("Temp: %f Cº       Hum: %f\n", temp, hum);
        snprintf(hum_display, 16, "Hum: %.2f", hum);
        snprintf(hum_display, 16, "Temp: %.2fºC", temp);
        ssd1306_clear_screen(&ssd1306, false);
        ssd1306_display_text(&ssd1306, 1, temp_display, 16, false);
        ssd1306_display_text(&ssd1306, 3, hum_display, 16, false);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}