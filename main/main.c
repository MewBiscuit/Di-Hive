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
#include "esp_netif_sntp.h"

//Logs
#define TAG "MAIN"

//Telemetry
#define SERVER "mqtt://192.168.43.229"
#define TOKEN "4IdMOqwMILd426ebULui"
#define TOPIC "v1/devices/me/telemetry"
#define PERIOD 30000
esp_mqtt_client_handle_t tb_client;
int port = 1883;

//NVS
char ssid_var[256] = "dummy_data";
char password_var[250] = "dummy_data";

//WiFi
#define AP_NAME "Di-Core_Prov"
#define AP_PWD "Provisioning123"
bool provisioned = false, credentials = false, wifi_flag = false, time_set = false;
int max_connections = 4, channel = 1;

//Local memory
char *file_data = "/sdcard/data.txt", *file_credentials = "/sdcard/credentials.txt";
bool sd_flag = 0, nvs_flag = 0;

//Sensors
bool mic_flag = false, ambient_in_flag = false, ambient_out_flag = false, weight_flag = false;

//SSD1306
SSD1306_t ssd1306;
int num_err = 0;

//Functions
void init_sensors(hx711_t *sens) {
    esp_err_t err;

    err = HX711_init(sens);
    if(err != ESP_OK) {
        weight_flag = true;
        ssd1306_display_text(&ssd1306, num_err, "HX711 failure", 13, false);
    }

    err = mic_setup(INMP441, GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_19);
    if(err != ESP_OK) {
        mic_flag = true;
        ssd1306_display_text(&ssd1306, num_err, "Mic failure", 11, false);
    }

    err = i2c_setup(I2C_NUM_0, I2C_MODE_MASTER, GPIO_NUM_22, GPIO_NUM_21, I2C_DEFAULT_FREQ);
    if(err != ESP_OK) {
        ambient_in_flag = true;
        ssd1306_display_text(&ssd1306, num_err, "Amb In failure", 14, false);
    }

    err = i2c_setup(I2C_NUM_1, I2C_MODE_MASTER, GPIO_NUM_26, GPIO_NUM_27, I2C_DEFAULT_FREQ);
    if(err != ESP_OK) {
        ambient_out_flag = true;
        ssd1306_display_text(&ssd1306, num_err, "Amb Out failure", 15, false);
    }
}

void turnoff_system(int line) {
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_25, 1);
    gpio_hold_en(GPIO_NUM_32); //Led/Screen PIN
    ssd1306_display_text(&ssd1306, line, "Critical failure", 16, false);
    gpio_deep_sleep_hold_en();
    esp_deep_sleep_start();
}

//Main
void app_main() {
    int i, num_saved_lines = 0;
    float sound = 0, temp_in = 0, temp_out = 0, hum_in = 0, hum_out = 0, weight = 0;
    time_t stamp = 0;
    esp_err_t err = ESP_OK;
    hx711_t load_cells = {.dout = GPIO_NUM_14, .pd_sck = GPIO_NUM_15, .gain = HX711_GAIN_A_128};;
    const uint_fast8_t data_size = 256;
    char data[data_size];
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    
    esp_netif_sntp_init(&config);

    i2c_master_init(&ssd1306, 32, 33, CONFIG_RESET_GPIO);
	ssd1306_init(&ssd1306, 128, 64);
    ssd1306_contrast(&ssd1306, 0xff);
    ssd1306_clear_screen(&ssd1306, false);

    for(i = 0, err = init_nvs(); i < 10 && err != ESP_OK; i++) {
        err = init_nvs();
    }

    //If we can't initiate NVS, restart. If already restarted, NVS is deemed unusable.
    if(err != ESP_OK) {
        if(esp_rom_get_reset_reason(0) == RESET_REASON_CORE_SW) {
            nvs_flag = 1;
            ssd1306_display_text(&ssd1306, num_err, "NVS Error", 9, false);
            num_err++;
        }
        else {
            esp_restart();
        }
    }

    else{
        if(read_string_from_nvs("time", ssid_var) == ESP_OK)
            time_set = true;
        if(read_string_from_nvs("ssid", ssid_var) == ESP_OK && read_string_from_nvs("password", password_var) == ESP_OK)
            credentials = true;
    }
    
    err = init_sd();
    if(err != ESP_OK) {
        sd_flag = 1;
        ssd1306_display_text(&ssd1306, num_err, "SD Error", 8, false);
        num_err++;
    }

    else if(!credentials)
        read_sd_creds(&credentials, ssid_var, password_var);

    if(sd_flag && nvs_flag) //System deemed unusable, turnoff sequence
        turnoff_system(num_err);

    //If system is usable
    init_sensors(&load_cells);
    esp_sleep_enable_timer_wakeup(PERIOD * 1000);

    //WiFi initialization
    err = wifi_init();
    if(err != ESP_OK) {
        wifi_flag = true;
        ssd1306_display_text(&ssd1306, num_err, "WiFi Error", 10, false);
        num_err++;
    }
    
    //3 states, we have access to WiFi module, no access to wifi module but sd, or not usable
    //If WiFi module works
    if(!wifi_flag) {
        if(credentials) { //We found credentials
            err = connect_ap(ssid_var, password_var);
            if (err != ESP_OK) {
                setup_ap(AP_NAME, AP_PWD, &channel, &max_connections);
            }
        }

        while(!is_connected()) {
            start_provisioning();
            while(!provisioned) {
                if(!sd_flag) {
                    if(ambient_in_flag) {
                        if(SHT40_read(I2C_NUM_0, &temp_in, &hum_in) != ESP_OK) {
                            temp_in = 0;
                            hum_in = 0;
                            ambient_in_flag = true;
                            ssd1306_display_text(&ssd1306, num_err, "Amb In failure", 14, false);
                            num_err++;
                        }
                    }

                    if(!ambient_out_flag) {
                        if(SHT40_read(I2C_NUM_1, &temp_out, &hum_out) != ESP_OK) {
                            temp_out = 0;
                            hum_out = 0;
                            ambient_out_flag = true;
                            ssd1306_display_text(&ssd1306, num_err, "Amb Out failure", 15, false);
                            num_err++;
                        }
                    }
                        
                    if(!mic_flag) {
                        if(read_audio(&sound) != ESP_OK) {
                            sound =  0;
                            mic_flag = true;
                            ssd1306_display_text(&ssd1306, num_err, "Mic failure", 11, false);
                            num_err++;
                        }
                    }

                    if(!weight_flag) {
                        if(HX711_read(&load_cells, &weight) != ESP_OK) {
                            weight = 0;
                            weight_flag = true;
                            ssd1306_display_text(&ssd1306, num_err, "HX711 failure", 13, false);
                            num_err++;
                        }
                    }
                        
                    time(&stamp);
                    snprintf(data, data_size, "{'ts':%lld, 'values':{'temperature_out':%f, 'temperature_in':%f, 'humidity_out':%f, 'humidity_in':%f, 'weight':%f, 'sound':%f}}", stamp, temp_out, temp_in, hum_out, hum_in, weight, sound);
                    write_data(file_data, data);
                    num_saved_lines++;
                }
                vTaskDelay(PERIOD / portTICK_PERIOD_MS);
                is_provisioned(&provisioned);
            }
            err = connect_ap(ssid_var, password_var);
            if (err != ESP_OK) {
                setup_ap(AP_NAME, AP_PWD, &channel, &max_connections);
            }
        }

        if(!time_set) {
            esp_netif_sntp_init(&config);
            
            if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(15000)) != ESP_OK) {
                ESP_LOGE(SD_TAG, "Failed to update system time within 15s timeout");
            }

            time(&stamp);
            setenv("TZ", "GMT+2", 1);
            tzset();
        }

        //Dump local data to server and clear SD card (except credentials)
        dump_data(file_data, TOPIC, tb_client, num_saved_lines, PERIOD, time_set);
        time_set = true;

        //Read and send data
        SHT40_read(I2C_NUM_0, &temp_in, &hum_in);
        SHT40_read(I2C_NUM_1, &temp_out, &hum_out);
        read_audio(&sound);
        HX711_read(&load_cells, &weight);
        post_numerical_data("weight", &weight, TOPIC, tb_client);
        post_numerical_data("temperature_in", &temp_in, TOPIC, tb_client);
        post_numerical_data("humidity_in", &hum_in, TOPIC, tb_client);
        post_numerical_data("temperature_out", &temp_out, TOPIC, tb_client);
        post_numerical_data("humidity_out", &hum_out, TOPIC, tb_client);
        post_numerical_data("sound", &sound, TOPIC, tb_client);

        //Check for updates
        check_updates();

        //Send Di-Core to sleep
        time(&stamp);
        snprintf(data, data_size, "%lld", stamp);
        write_string_to_nvs("time", data);   
        esp_deep_sleep_start();
    }

    //Wifi module out but sd card works
    else if(!sd_flag){
        if(ambient_in_flag) {
            if(SHT40_read(I2C_NUM_0, &temp_in, &hum_in) != ESP_OK) {
                temp_in = 0;
                hum_in = 0;
                ambient_in_flag = true;
                ssd1306_display_text(&ssd1306, num_err, "Amb In failure", 14, false);
                num_err++;
            }
        }

        if(!ambient_out_flag) {
            if(SHT40_read(I2C_NUM_1, &temp_out, &hum_out) != ESP_OK) {
                temp_out = 0;
                hum_out = 0;
                ambient_out_flag = true;
                ssd1306_display_text(&ssd1306, num_err, "Amb Out failure", 15, false);
                num_err++;
            }
        }
            
        if(!mic_flag) {
            if(read_audio(&sound) != ESP_OK) {
                sound =  0;
                mic_flag = true;
                ssd1306_display_text(&ssd1306, num_err, "Mic failure", 11, false);
                num_err++;
            }
        }

        if(!weight_flag) {
            if(HX711_read(&load_cells, &weight) != ESP_OK) {
                weight = 0;
                weight_flag = true;
                ssd1306_display_text(&ssd1306, num_err, "HX711 failure", 13, false);
                num_err++;
            }
        }
        time(&stamp);
        snprintf(data, data_size, "{'ts':%lld, 'values':{'temperature_out':%f, 'temperature_in':%f, 'humidity_out':%f, 'humidity_in':%f, 'weight':%f, 'sound':%f}}", stamp, temp_out, temp_in, hum_out, hum_in, weight, sound);
        write_data(file_data, data);
        esp_deep_sleep_start();
    }

    //No WiFi and no SD Card -> Not useful, shutdown
    else 
        turnoff_system(num_err);
}