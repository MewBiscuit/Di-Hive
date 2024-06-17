//Homebrewed libraries
#include "nvs_man.h"
#include "wifi_man.h"
#include "mqtt_man.h"
#include "sensors_man.h"
#include "ota_man.h"
#include "sd_man.h"

//External libraries
#include "ssd1306.h"
#include "font8x8_basic.h"

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

//Sensors
bool mic_flag = false, ambient_in_flag = false, ambient_out_flag = false, weight_flag = false;

//Functions
void init_sensors(hx711_t *sens) {
    esp_err_t err;

    err = HX711_init(sens);
    if(err != ESP_OK) {
        weight_flag = true;
    }

    err = mic_setup(INMP441);
    if(err != ESP_OK) {
        mic_flag = true;
    }

    err = i2c_setup(I2C_NUM_0, I2C_MODE_MASTER, GPIO_NUM_22, GPIO_NUM_21, I2C_DEFAULT_FREQ);
    if(err != ESP_OK) {
        ambient_in_flag = true;
    }

    err = i2c_setup(I2C_NUM_1, I2C_MODE_MASTER, GPIO_NUM_26, GPIO_NUM_27, I2C_DEFAULT_FREQ);
    if(err != ESP_OK) {
        ambient_out_flag = true;
    }
}

void turnoff_system(SSD1306_t *dev, int line) {
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_25, 1);
    gpio_hold_en(GPIO_NUM_32); //Led/Screen PIN
    ssd1306_display_text(dev, line, "Critical failure", 16, false);
    gpio_deep_sleep_hold_en();
    esp_deep_sleep_start();
}

void app_main() {
    esp_err_t err;
    const uint_fast8_t data_size = 256;
    char data[data_size];
    float sound = 0, temp_in = 0, temp_out = 0, hum_in = 0, hum_out = 0, weight = 0;
    time_t stamp = 0;

    err = init_sd();
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Couldn't initialize SD module");
        return;
    }

    else{
        ESP_LOGI(TAG, "Initialized SD module");
    }

    time(&stamp);
    snprintf(data, data_size, "{'ts':%lld, 'values':{'temperature_out':%f, 'temperature_in':%f, 'humidity_out':%f, 'humidity_in':%f, 'weight':%f, 'sound':%f}}", stamp, temp_out, temp_in, hum_out, hum_in, weight, sound);
    write_data(file_data, data);
    ESP_LOGI(TAG, "Wrote data to sd, check internally");
    return;
}