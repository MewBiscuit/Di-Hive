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
const int port = 1883;

//NVS
char ssid_var[256] = "dummy_data";
char password_var[250] = "dummy_data";

//WiFi
bool provisioned = false;
int max_connections = 4;
int channel = 1;

//Telemetry
esp_mqtt_client_handle_t tb_client;

//FreeRTOS
#define STACK_SIZE 200
StaticTask_t xTaskBuffer;
StackType_t xStack[STACK_SIZE];

// Weight reading, saving, and telemetry sending task
void weightTask(void* pvParameters) {
    float weight = 0;
    time_t stamp;
    Sensor hx711 = {.sda = GPIO_NUM_33, .sck = GPIO_NUM_14};
    int ms_periodicity = 600000;

    HX711_init(hx711);

    for(;;){
        for(;provisioned;) {
            HX711_read(hx711, &weight);
            post_numerical_data("weight", &weight, TOPIC, tb_client);
            vTaskDelay(ms_periodicity / portTICK_PERIOD_MS);
        }

        for(;!provisioned;){
            HX711_read(hx711, &weight);
            //TODO: save to SD
            vTaskDelay(ms_periodicity / portTICK_PERIOD_MS);
        }
        //TODO: dump sdcard data and free space
    }
}

//Task for sht40 sensor reading, saving and telemetry sending
//TODO: separate inside and outside components into 2 tasks
void ambientVarsTask (void* pvParameters) {
    float temp_in = 0, temp_out = 0, hum_in = 0, hum_out = 0;
    time_t stamp;
    int ms_periodicity = 30000;

    i2c_setup(I2C_NUM_0, I2C_MODE_MASTER, GPIO_NUM_22, GPIO_NUM_21, I2C_DEFAULT_FREQ);
    i2c_setup(I2C_NUM_1, I2C_MODE_MASTER, GPIO_NUM_26, GPIO_NUM_27, I2C_DEFAULT_FREQ);

    for(;;){
        for(;provisioned;) {
            SHT40_read(I2C_NUM_0, &temp_in, &hum_in);
            SHT40_read(I2C_NUM_1, &temp_out, &hum_out);
            post_numerical_data("temperature_in", &temp_in, TOPIC, tb_client);
            post_numerical_data("humidity_in", &hum_in, TOPIC, tb_client);
            post_numerical_data("temperature_out", &temp_out, TOPIC, tb_client);
            post_numerical_data("humidity_out", &hum_out, TOPIC, tb_client);
            vTaskDelay(ms_periodicity / portTICK_PERIOD_MS);
        }

        for(;!provisioned;){
            SHT40_read(I2C_NUM_0, &temp_in, &hum_in);
            SHT40_read(I2C_NUM_1, &temp_out, &hum_out);
            //TODO: save to SD
            vTaskDelay(ms_periodicity / portTICK_PERIOD_MS);
        }

        //TODO: dump sdcard data and free space
    }
}

void soundTask(void* pvParameters) {
    float sound = 0;
    time_t stamp;
    int ms_periodicity = 10000;

    mic_setup(INMP441);
    
    for(;;){
        for(;provisioned;) {
            read_audio(&sound);
            post_numerical_data("sound", &sound, TOPIC, tb_client);
            vTaskDelay(ms_periodicity / portTICK_PERIOD_MS);
        }

        for(;!provisioned;){
            read_audio(&sound);
            //TODO: save to SD
            vTaskDelay(ms_periodicity / portTICK_PERIOD_MS);
        }
        //TODO: dump sdcard data and free space
    }
}


//TODO: Implement all tasks of system, including OTA and telemetry

void app_main() {
    float sound = 0, temp_in = 0, temp_out = 0, hum_in = 0, hum_out = 0;
    char flag_local_data;
    int i;
    esp_err_t err = ESP_OK;
    TaskHandle_t weightTaskHandle = NULL;

    for(i = 0; i < 10 && init_nvs() != ESP_OK; i++){
        ESP_LOGE(TAG, "Could not initialize nvs: %d", err);
        esp_restart();
    }

    //WiFi credential reading process
    err = read_string_from_nvs("ssid", ssid_var);
    if(err != ESP_OK) {
        ESP_LOGI(TAG, "No SSID found in NVS");
    }
    err = read_string_from_nvs("password", password_var);
    if(err != ESP_OK) {
        ESP_LOGI(TAG, "No password found in NVS");
    }
    err = read_string_from_nvs("flag_local_data", &flag_local_data);
    if(err != ESP_OK) {
        write_string_to_nvs("flag_local_data", "0");
        flag_local_data = '0';
    }

    i2c_setup(I2C_NUM_0, I2C_MODE_MASTER, GPIO_NUM_22, GPIO_NUM_21, I2C_DEFAULT_FREQ);
    i2c_setup(I2C_NUM_1, I2C_MODE_MASTER, GPIO_NUM_26, GPIO_NUM_27, I2C_DEFAULT_FREQ);
    mic_setup(INMP441);



    //Connect to WiFi
    wifi_init();
    err = connect_ap(ssid_var, password_var);
    if (err != ESP_OK) {
        write_string_to_nvs("flag_local_data", "1");
        flag_local_data = '1';
        setup_ap("Di-Core_Prov", "provisioning112", &channel, &max_connections);
    }


    //TODO: Transform to Task
    while (err != ESP_OK) {
        is_provisioned(&provisioned);
        printf("Checked if provisioned\n");
        for(;!provisioned;) {
            is_provisioned(&provisioned);
            printf("Checked if provisioned in loop\n");
            //Read sensor data
            SHT40_read(I2C_NUM_0, &temp_in, &hum_in);
            printf("Read SHT40 1\n");
            SHT40_read(I2C_NUM_1, &temp_out, &hum_out);
            printf("Read SHT40 2\n");
            read_audio(&sound);
            printf("Read mic\n");

            //Save data to SD card
            //time(&stamp);
            //snprintf(data, MAX_CHAR_SIZE, "{'ts':%lld, 'values':{'%s':%d, '%s':%d, '%s':%d, '%s':%d, '%s':%d}}", stamp, "temperature_out", temp_out,
            //"temperature_in", temp_in, "humidity_in", hum_in, "noise", noise, "weight", weight);

            printf("{'%s':%2.f, '%s':%2.f, '%s':%2.f, '%s':%2.f, '%s':%2.f, '%s':%2.f}}\n", "temperature_out", temp_out, "temperature_in", temp_in, "humidity_in", hum_in, "humidity_out", hum_out, "sound", sound, "weight", weight);

            //Can't send to sleep in order to keep prov server running
            vTaskDelay(30000 / portTICK_PERIOD_MS);
        }
        err = read_string_from_nvs("ssid", ssid_var);
        if(err != ESP_OK) {
            ESP_LOGI(TAG, "No SSID found in NVS");
        }
        err = read_string_from_nvs("password", password_var);
        if(err != ESP_OK) {
            ESP_LOGI(TAG, "No password found in NVS");
        }
        err = connect_ap(ssid_var, password_var);
    }

    //Connect to Thingsboard
    tb_client = connect_mqtt_token(SERVER, &port, TOKEN);

    if(flag_local_data == '1') {
        //TODO: Send all logged data to Thingsboard
        
        // All data sent, unmount partition and disable SDMMC peripheral
        ESP_LOGI(TAG, "Card unmounted");
        
        //Turn off flag_local_data
        write_string_to_nvs("flag_local_data", "0");
    }

    //Setup wakeup timer for sleep mode
    esp_sleep_enable_timer_wakeup(25000000);

    // Every 30 seconds we read all of the sensors and dump the data
    SHT40_read(I2C_NUM_0, &temp_in, &hum_in);
    SHT40_read(I2C_NUM_1, &temp_out, &hum_out);
    read_audio(&sound);
    HX711_read(hx711, &weight);

    post_numerical_data("temperature_in", &temp_in, TOPIC, tb_client);
    post_numerical_data("humidity_in", &hum_in, TOPIC, tb_client);
    post_numerical_data("temperature_out", &temp_out, TOPIC, tb_client);
    post_numerical_data("humidity_out", &hum_out, TOPIC, tb_client);
    post_numerical_data("sound", &sound, TOPIC, tb_client);
    post_numerical_data("weight", &weight, TOPIC, tb_client);

    vTaskDelay(5000 / portTICK_PERIOD_MS); // Wait 5 seconds to avoid sleeping before sending data
    wifi_release();
    esp_deep_sleep_start();
}