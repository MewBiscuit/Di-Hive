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
    esp_err_t err = ESP_OK;

    for(i = 0, err = init_nvs(); i < 10 && err != ESP_OK; i++) {
        err = init_nvs();
    }

    //If we can't initiate NVS, restart. If already restarted, NVS is deemed unusable.
    if(err != ESP_OK) {
        if(rtc_get_reset_reason(0) == SW_CPU_RESET) {
            nvs_flag = 1;
        }
        else {
            esp_restart();
        }
    }

    else if(read_string_from_nvs("ssid", ssid_var) == ESP_OK && read_string_from_nvs("password", password_var) == ESP_OK)
        credentials = true;

    err = init_sd();
    if(err != ESP_OK)
        sd_flag = 1;

    else if(!credentials)
        read_sd_creds(&credentials, ssid_var, password_var);

    //WiFi initialization
    err = wifi_init();
    if(err != ESP_OK)
        wifi_flag = true;
    
    //2 states, we have access to WiFi module or we don't
    //If WiFi module works
    if(!wifi_flag) {
        if(credentials) { //We found credentials
            err = connect_ap(ssid_var, password_var);
            if (err != ESP_OK) {
                setup_ap(AP_NAME, AP_PWD, &channel, &max_connections);
                printf("Setup AP\n");
            }
        }

        while(!connected) {
            while(!provisioned) {
                vTaskDelay(30000 / portTICK_PERIOD_MS);
                is_provisioned(&provisioned);
            }
            printf("Device was provisioned\n");
            err = connect_ap(ssid_var, password_var);
            if (err != ESP_OK) {
                setup_ap(AP_NAME, AP_PWD, &channel, &max_connections);
                printf("Setup AP\n");
            }
        }

        //Check for updates
        printf("Checking for updates");
        check_updates();
        esp_ota_mark_app_valid_cancel_rollback();
        esp_restart();
    }
}