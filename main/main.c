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

//ESP_Provisioning
bool provisioned = false;
int max_connections = 4;
int channel = 1;

void app_main() {

}