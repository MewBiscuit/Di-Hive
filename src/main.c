//Homebrewed libraries
#include "nvs_man.h"
#include "wifi_man.h"
#include "mqtt_man.h"
#include "sensors_man.h"
#include "ota_man.h"

//Standard libraries
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "esp_task_wdt.h"

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
bool provisioned = false, set_ap = false;
int max_connections = 4, channel = 1;

//Telemetry
esp_mqtt_client_handle_t tb_client;

//FreeRTOS
#define STACK_SIZE 8192
StaticTask_t weightTaskBuffer, ambientTaskBuffer, soundTaskBuffer, otaTaskBuffer;
StackType_t weightStack[STACK_SIZE], ambientStack[STACK_SIZE], soundStack[STACK_SIZE], otaStack[STACK_SIZE];
portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED;

//SD_Card
sdmmc_card_t *card;


// FUNCTIONS 

esp_err_t init_sd(){
    esp_err_t err;
    const char mount_point[] = "/sdcard";
    const char *file_ambient = "/sdcard/ambient.txt", *file_sound = "/sdcard/sound.txt", *file_weight = "/sdcard/weight.txt";
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    FILE *f;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5
    };

    slot_config.width = 4;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    err = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Could not initialize SD_Card module");
        return err;
    }

    f = fopen(file_ambient, "w");
    fclose(f);
    f = fopen(file_sound, "w");
    fclose(f);
    f = fopen(file_weight, "w");
    fclose(f);

    return err;
}

void write_data(char* path, char* data){
    FILE *f;

    f = fopen(*path, "a");
    fprintf(f, *data, card->cid.name);
    fclose(f);

    return;
}

void dump_data(char* path){
    FILE *f;
    const uint_fast8_t data_size = 256;
    char data[data_size];

    f = fopen(*path, "r");
    while (fgets(data, sizeof(data), f)) {
            // Send the data to the server
            post_line(data, TOPIC, tb_client);
    }

    fclose(f);
    unlink(f);
    f = fopen(*path, "w");
    fclose(f);

    return;
}

// TASKS

// Weight reading, saving, and telemetry sending task
void weightTask(void* pvParameters) {
    float weight = 0;
    time_t stamp = 0;
    const uint_fast8_t data_size = 64;
    char data[data_size];
    const char *filename = "/sdcard/weight.txt";
    Sensor hx711 = {.sda = GPIO_NUM_33, .sck = GPIO_NUM_14};
    int ms_periodicity = 600000;

    HX711_init(hx711);

    while(true){
        while(connected) {
            HX711_read(hx711, &weight);
            taskENTER_CRITICAL(&mutex);
            post_numerical_data("weight", &weight, TOPIC, tb_client);
            taskEXIT_CRITICAL(&mutex);
            ESP_ERROR_CHECK(esp_task_wdt_reset());  // Reset watchdog timer
            vTaskDelay(ms_periodicity / portTICK_PERIOD_MS);
        }

        while(!connected) {
            HX711_read(hx711, &weight);
            time(&stamp);
            snprintf(data, data_size, "{'ts':%ld, 'values':{'weight':%f}}", stamp, weight);
            //write_data(filename, data);
            taskENTER_CRITICAL(&mutex);
            if(!provisioned) {
                if(!set_ap) {
                    setup_ap(AP_NAME, AP_PWD, &channel, &max_connections);
                    set_ap = true;
                }
                is_provisioned(&provisioned);
            }
            if(provisioned && !connected) {
                read_string_from_nvs("ssid", ssid_var);
                read_string_from_nvs("password", password_var);
                connect_ap(ssid_var, password_var);
            }
            taskEXIT_CRITICAL(&mutex);
            ESP_ERROR_CHECK(esp_task_wdt_reset());  // Reset watchdog timer
            vTaskDelay(ms_periodicity / portTICK_PERIOD_MS);
        }
        taskENTER_CRITICAL(&mutex);
        //dump_data(filename);
        set_ap = false;
        taskEXIT_CRITICAL(&mutex);
    }
}

//Task for sht40 sensor reading, saving and telemetry sending
void ambientTask(void* pvParameters) {
    float temp_in = 0, temp_out = 0, hum_in = 0, hum_out = 0;
    time_t stamp = 0;
    const uint_fast8_t data_size = 128;
    char data[data_size];
    const char *filename = "/sdcard/ambient.txt";
    int ms_periodicity = 30000;

    i2c_setup(I2C_NUM_0, I2C_MODE_MASTER, GPIO_NUM_22, GPIO_NUM_21, I2C_DEFAULT_FREQ);
    i2c_setup(I2C_NUM_1, I2C_MODE_MASTER, GPIO_NUM_26, GPIO_NUM_27, I2C_DEFAULT_FREQ);

    while(true){
        while(connected) {
            SHT40_read(I2C_NUM_0, &temp_in, &hum_in);
            SHT40_read(I2C_NUM_1, &temp_out, &hum_out);
            taskENTER_CRITICAL(&mutex);
            post_numerical_data("temperature_in", &temp_in, TOPIC, tb_client);
            post_numerical_data("humidity_in", &hum_in, TOPIC, tb_client);
            post_numerical_data("temperature_out", &temp_out, TOPIC, tb_client);
            post_numerical_data("humidity_out", &hum_out, TOPIC, tb_client);
            taskEXIT_CRITICAL(&mutex);
            ESP_ERROR_CHECK(esp_task_wdt_reset());  // Reset watchdog timer
            vTaskDelay(ms_periodicity / portTICK_PERIOD_MS);
        }

        while(!connected) {
            SHT40_read(I2C_NUM_0, &temp_in, &hum_in);
            SHT40_read(I2C_NUM_1, &temp_out, &hum_out);
            time(&stamp);
            snprintf(data, data_size, "{'ts':%ld, 'values':{'temperature_out':%f, 'temperature_in':%f, 'humidity_out':%f, 'humidity_in':%f}}", stamp, temp_out, temp_in, hum_out, hum_in);
            //write_data(filename, data);
            taskENTER_CRITICAL(&mutex);
            if(!provisioned) {
                if(!set_ap) {
                    setup_ap(AP_NAME, AP_PWD, &channel, &max_connections);
                    set_ap = true;
                }
                is_provisioned(&provisioned);
            }
            if(provisioned && !connected) {
                read_string_from_nvs("ssid", ssid_var);
                read_string_from_nvs("password", password_var);
                connect_ap(ssid_var, password_var);
            }
            taskEXIT_CRITICAL(&mutex);
            ESP_ERROR_CHECK(esp_task_wdt_reset());  // Reset watchdog timer
            vTaskDelay(ms_periodicity / portTICK_PERIOD_MS);
        }
        taskENTER_CRITICAL(&mutex);
        //dump_data(filename);
        set_ap = false;
        taskEXIT_CRITICAL(&mutex);
    }
}

void soundTask(void* pvParameters) {
    float sound = 0;
    time_t stamp = 0;
    const uint_fast8_t data_size = 128;
    char data[data_size];
    const char *filename = "/sdcard/sound.txt";
    int ms_periodicity = 5000;

    mic_setup(INMP441);
    
    while(true){
        while(connected) {
            read_audio(&sound);
            taskENTER_CRITICAL(&mutex);
            post_numerical_data("sound", &sound, TOPIC, tb_client);
            taskEXIT_CRITICAL(&mutex);
            ESP_ERROR_CHECK(esp_task_wdt_reset());  // Reset watchdog timer
            vTaskDelay(ms_periodicity / portTICK_PERIOD_MS);
        }

        while(!connected) {
            read_audio(&sound);
            time(&stamp);
            snprintf(data, data_size, "{'ts':%ld, 'values':{'sound':%f}}", stamp, sound);
            //write_data(filename, data);
            taskENTER_CRITICAL(&mutex);
            if(!provisioned) {
                if(!set_ap) {
                    setup_ap(AP_NAME, AP_PWD, &channel, &max_connections);
                    set_ap = true;
                }
                is_provisioned(&provisioned);
            }
            if(provisioned && !connected) {
                read_string_from_nvs("ssid", ssid_var);
                read_string_from_nvs("password", password_var);
                connect_ap(ssid_var, password_var);
            }
            taskEXIT_CRITICAL(&mutex);
            ESP_ERROR_CHECK(esp_task_wdt_reset());  // Reset watchdog timer
            vTaskDelay(ms_periodicity / portTICK_PERIOD_MS);
        }
        taskENTER_CRITICAL(&mutex);
        //dump_data(filename);
        set_ap = false;
        taskEXIT_CRITICAL(&mutex);
    }
}

void otaTask(void* pvParameters) {
    esp_err_t err;

    while(true) {
            ESP_LOGI(TAG, "Checking for OTA updates...");
            err = check_updates();
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "OTA update completed successfully");
            } 
            
            else {
                ESP_LOGE(TAG, "OTA update failed");
            }
            ESP_ERROR_CHECK(esp_task_wdt_reset());
            vTaskDelay(3600000 / portTICK_PERIOD_MS);  // Check for updates every hour
    }
}

void app_main() {
    int i;
    esp_err_t err = ESP_OK;

    for(i = 0, err = init_nvs(); i < 10 && err != ESP_OK; i++) {
        ESP_LOGE(TAG, "Could not initialize nvs: %d", err);
        err = init_nvs();
    }

    if(err != ESP_OK) {
        if(rtc_get_reset_reason(0) == SW_CPU_RESET) {
            ESP_LOGI(TAG, "Unable to initialize NVS, stand-by. Saving data locally");
            //TODO: Should turn on a LED or something to represent error, need to figure that out
        }
        else {
            ESP_LOGI(TAG, "Couldn't start up NVS, restarting");
            esp_restart();
        }
    }

    //We can initalize WiFi
    else {
        err = wifi_init();

        //WiFi credential reading process
        read_string_from_nvs("ssid", ssid_var);
        read_string_from_nvs("password", password_var);

        //Connect to WiFi
        err = connect_ap(ssid_var, password_var);
        if (err != ESP_OK) {
            setup_ap(AP_NAME, AP_PWD, &channel, &max_connections);
        }
    }

    //Launch all tasks
    xTaskCreateStatic(otaTask, "ota", STACK_SIZE, ( void * ) 1, 1, otaStack, &otaTaskBuffer);
    ESP_ERROR_CHECK(esp_task_wdt_add(xTaskCreateStatic(soundTask, "sound", STACK_SIZE, ( void * ) 1, 4, soundStack, &soundTaskBuffer)));
    ESP_ERROR_CHECK(esp_task_wdt_add(xTaskCreateStatic(ambientTask, "ambient", STACK_SIZE, ( void * ) 1, 3, ambientStack, &ambientTaskBuffer)));
    ESP_ERROR_CHECK(esp_task_wdt_add(xTaskCreateStatic(weightTask, "weight", STACK_SIZE, ( void * ) 1, 2, weightStack, &weightTaskBuffer)));
}