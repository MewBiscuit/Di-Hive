//ESP specific libraries
#include "esp_sleep.h"

//Homebrewed libraries
#include "nvs_man.h"
#include "wifi_man.h"
#include "mqtt_man.h"
#include "sensors_man.h"
#include "ota_man.h"

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
    float decibels = 0.0, average = 0;
    char dB_display[16];
    int i;

    mic_setup(INMP441);

    for(; 1;) {
        read_audio(&decibels);
        printf(">raw:%.3f\n", decibels);
        average += decibels;
        snprintf(dB_display, 16, "%.1f dB", decibels);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}