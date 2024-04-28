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
    float weight;
    SSD1306_t ssd1306;
    char weight_display[16];

    Sensor hx771 = {
        .sda = GPIO_NUM_22,
        .sck = GPIO_NUM_21
    };

    HX711_init(hx771);

    i2c_master_init(&ssd1306, 32, 33, CONFIG_RESET_GPIO);
	ssd1306_init(&ssd1306, 128, 64);
    ssd1306_contrast(&ssd1306, 0xff);
    ssd1306_clear_screen(&ssd1306, false);



    for(; 1;) {
        HX711_read(hx771, &weight);
        printf("%.3f kg\n", weight);
        snprintf(weight_display, 16, "%.3f kg", weight);
        ssd1306_clear_line(&ssd1306, 1, false);
        ssd1306_display_text(&ssd1306, 1, weight_display, 16, false);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}