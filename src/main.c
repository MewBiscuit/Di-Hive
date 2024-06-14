//Homebrewed libraries
#include "nvs_man.h"
#include "wifi_man.h"
#include "mqtt_man.h"
#include "sensors_man.h"
#include "ota_man.h"
#include "sd_man.h"

//Standard libraries
#include "esp_sleep.h"

//Logs
#define TAG "MAIN"

//Telemetry
#define SERVER "mqtt://demo.thingsboard.io"
#define TOKEN ""
#define TOPIC "v1/devices/me/telemetry"
const int port = 1883;

//NVS
char ssid_var[256] = "dummy_data";
char password_var[250] = "dummy_data";

//WiFi
#define AP_NAME "Di-Core_Prov"
#define AP_PWD "Provisioning123"
bool provisioned = false, credentials = false, wifi_flag = false;
int max_connections = 4, channel = 1;

//Telemetry
esp_mqtt_client_handle_t tb_client;

//Local memory
char *file_data = "/sdcard/data.txt", *file_credentials = "/sdcard/credentials.txt";
bool sd_flag = 0, nvs_flag = 0;

void app_main() {
    int i;

    for(i = 0; i < 10 && init_nvs() != ESP_OK; i++) {
        printf("NVS Init method failed for %d time(s)\n", i);
    }

    //Init wifi module
    if(wifi_init() != ESP_OK) {
        printf("Failed to initialize Wifi Module\n");
        return;
    }

    //Connect to AP
    while(!is_connected()) {
        printf("Attempting to connect to AP...\n");
        connect_ap("IA", "hola1112");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    printf("Connected!\nChecking updates...\n");
    
    //Check for updates
    if(check_updates() == ESP_OK) {
        printf("Succesfully updated :)\n");
    }

    else {
        printf("Something went wrong my man :C, check the logs\n");
    }
}