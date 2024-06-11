//Homebrewed libraries
#include "nvs_man.h"
#include "wifi_man.h"
#include "mqtt_man.h"
#include "sensors_man.h"
#include "ota_man.h"

//Standard libraries
#include "esp_sleep.h"

//Logs
#define TAG "MAIN"

void app_main() {
    float weight = 0;
    int32_t raw_data;
    esp_err_t err = ESP_OK;
    hx711_t sensor = {.dout = GPIO_NUM_26, .pd_sck = GPIO_NUM_27, .gain = HX711_GAIN_A_128};

    printf("Initializing sensor...\n");
    err = HX711_init(&sensor);

    if(err != ESP_OK) {
        printf("Couldn't initialize hx771\n");
    }

    else{
        printf("Done\n");
        while(true) {
            printf("Reading sensor...\n");
            if(HX711_read(&sensor, &weight, &raw_data) != ESP_OK) {
                printf("Error reading hx771\n");
            }

            else {
                printf("Raw reading: %d g\n", raw_data);
            }

            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }
}