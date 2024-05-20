//Homebrewed libraries
#include "nvs_man.h"
#include "wifi_man.h"
#include "mqtt_man.h"
#include "sensors_man.h"
#include "ota_man.h"

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
bool provisioned = false;
int max_connections = 4, channel = 1;


//Telemetry
esp_mqtt_client_handle_t tb_client;

//FreeRTOS
#define STACK_SIZE 200
StaticTask_t weightTaskBuffer, ambientTaskBuffer, soundTaskBuffer;
StackType_t weightStack[STACK_SIZE], ambientStack[STACK_SIZE], soundStack[STACK_SIZE];
portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED;

// Weight reading, saving, and telemetry sending task
void weightTask(void* pvParameters) {
    float weight = 0;
    time_t stamp = 0;
    const uint_fast8_t data_size = 64;
    char data[data_size];
    Sensor hx711 = {.sda = GPIO_NUM_33, .sck = GPIO_NUM_14};
    int ms_periodicity = 600000;

    HX711_init(hx711);

    for(;;){
        for(;connected;) {
            HX711_read(hx711, &weight);
            post_numerical_data("weight", &weight, TOPIC, tb_client);
            vTaskDelay(ms_periodicity / portTICK_PERIOD_MS);
        }

        for(;!connected;) {
            HX711_read(hx711, &weight);
            time(&stamp);
            //TODO: save to SD
            snprintf(data, data_size, "{'ts':%ld, 'values':{'weight':%f}", stamp, weight);
            taskENTER_CRITICAL(&mutex);
            if(!provisioned) {
                is_provisioned(&provisioned);
            }
            if(provisioned && !connected) {
                read_string_from_nvs("ssid", ssid_var);
                read_string_from_nvs("password", password_var);
                connect_ap(ssid_var, password_var);
            }
            taskEXIT_CRITICAL(&mutex);
            vTaskDelay(ms_periodicity / portTICK_PERIOD_MS);
        }
        //TODO: dump sdcard data and free space
    }
}

//Task for sht40 sensor reading, saving and telemetry sending
void ambientTask (void* pvParameters) {
    float temp_in = 0, temp_out = 0, hum_in = 0, hum_out = 0;
    time_t stamp = 0;
    const uint_fast8_t data_size = 128;
    char data[data_size];
    int ms_periodicity = 30000;

    i2c_setup(I2C_NUM_0, I2C_MODE_MASTER, GPIO_NUM_22, GPIO_NUM_21, I2C_DEFAULT_FREQ);
    i2c_setup(I2C_NUM_1, I2C_MODE_MASTER, GPIO_NUM_26, GPIO_NUM_27, I2C_DEFAULT_FREQ);

    for(;;){
        for(;connected;) {
            SHT40_read(I2C_NUM_0, &temp_in, &hum_in);
            SHT40_read(I2C_NUM_1, &temp_out, &hum_out);
            post_numerical_data("temperature_in", &temp_in, TOPIC, tb_client);
            post_numerical_data("humidity_in", &hum_in, TOPIC, tb_client);
            post_numerical_data("temperature_out", &temp_out, TOPIC, tb_client);
            post_numerical_data("humidity_out", &hum_out, TOPIC, tb_client);
            vTaskDelay(ms_periodicity / portTICK_PERIOD_MS);
        }

        for(;!connected;) {
            SHT40_read(I2C_NUM_0, &temp_in, &hum_in);
            SHT40_read(I2C_NUM_1, &temp_out, &hum_out);
            time(&stamp);
            //TODO: save to SD
            snprintf(data, data_size, "{'ts':%ld, 'values':{'temperature_out':%f, 'temperature_in':%f, 'humidity_out':%f, 'humidity_in':%f}}", stamp, temp_out, temp_in, hum_out, hum_in);
            taskENTER_CRITICAL(&mutex);
            if(!provisioned) {
                is_provisioned(&provisioned);
            }
            if(provisioned && !connected) {
                read_string_from_nvs("ssid", ssid_var);
                read_string_from_nvs("password", password_var);
                connect_ap(ssid_var, password_var);
            }
            taskEXIT_CRITICAL(&mutex);
            vTaskDelay(ms_periodicity / portTICK_PERIOD_MS);
        }

        //TODO: dump sdcard data and free space
    }
}

void soundTask(void* pvParameters) {
    float sound = 0;
    time_t stamp = 0;
    const uint_fast8_t data_size = 128;
    char data[data_size];
    int ms_periodicity = 5000;

    mic_setup(INMP441);
    
    for(;;){
        for(;connected;) {
            read_audio(&sound);
            post_numerical_data("sound", &sound, TOPIC, tb_client);
            vTaskDelay(ms_periodicity / portTICK_PERIOD_MS);
        }

        for(;!connected;) {
            read_audio(&sound);
            time(&stamp);
            //TODO: save to SD
            snprintf(data, data_size, "{'ts':%ld, 'values':{'sound':%f}", stamp, sound);
            taskENTER_CRITICAL(&mutex);
            if(!provisioned) {
                is_provisioned(&provisioned);
            }

            if(provisioned && !connected) {
                read_string_from_nvs("ssid", ssid_var);
                read_string_from_nvs("password", password_var);
                connect_ap(ssid_var, password_var);
            }
            taskEXIT_CRITICAL(&mutex);
            vTaskDelay(ms_periodicity / portTICK_PERIOD_MS);
        }
        //TODO: dump sdcard data and free space
    }
}


//TODO: Implement OTA task
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
        connect_ap(ssid_var, password_var);
    }

    //Launch all tasks
    xTaskCreateStatic(soundTask, "sound", STACK_SIZE, ( void * ) 1, 1, soundStack, &soundTaskBuffer);
    xTaskCreateStatic(ambientTask, "ambient", STACK_SIZE, ( void * ) 1, 1, ambientStack, &ambientTaskBuffer);
    xTaskCreateStatic(weightTask, "weight", STACK_SIZE, ( void * ) 1, 1, weightStack, &weightTaskBuffer);
}