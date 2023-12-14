#include <stdio.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"


//https://github.com/thingsboard/thingsboard-client-sdk/blob/master/examples/0011-esp8266_esp32_subscribe_OTA_MQTT/0011-esp8266_esp32_subscribe_OTA_MQTT.ino
#include <Espressif_Updater.h>
#include <Espressif_MQTT_Client.h>
#include <ThingsBoard.h>

extern "C" {
    #include "nvs_man.h"
    #include "wifi_man.h"
    #include "adc_man.h"
    #include "sensors_man.h"
}

#define TAG "MAIN"
#define ENCRYPTED false
#define MAX_CHAR_SIZE    128
#define MOUNT_POINT "/sdcard"

constexpr char CURRENT_FIRMWARE_TITLE[] = "Di-Hive";
constexpr char CURRENT_FIRMWARE_VERSION[] = "0.1.0";

constexpr uint8_t FIRMWARE_FAILURE_RETRIES = 12U;
constexpr uint16_t FIRMWARE_PACKET_SIZE = 4096U;

constexpr char TOKEN[] = "g4gzei5ivlqn32g824lr";
constexpr char THINGSBOARD_SERVER[] = "demo.thingsboard.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;
constexpr uint16_t MAX_FW_SIZE = FIRMWARE_PACKET_SIZE + 50U;
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;

Espressif_MQTT_Client mqtt_client;
ThingsBoard tb(mqtt_client, MAX_FW_SIZE);
Espressif_Updater updater;

bool currentFWSent = false;
bool updateRequestSent = false;

void updatedCallback(const bool& success) {
  if (success) {
    ESP_LOGI(TAG, "Done, Reboot now");
    esp_restart();
    return;
  }
  ESP_LOGI(TAG, "Downloading firmware failed");
}

void progressCallback(const size_t& currentChunk, const size_t& totalChuncks) {
  ESP_LOGI("MAIN", "Downwloading firmware progress %.2f%%", static_cast<float>(currentChunk * 100U) / totalChuncks);
}

void OTA_process(){
    if (!currentFWSent) {
        currentFWSent = tb.Firmware_Send_Info(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION) && tb.Firmware_Send_State(FW_STATE_UPDATED);
    }

    if (!updateRequestSent) {
        const OTA_Update_Callback callback(&progressCallback, &updatedCallback, CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION, &updater, FIRMWARE_FAILURE_RETRIES, FIRMWARE_PACKET_SIZE);
        updateRequestSent = tb.Start_Firmware_Update(callback);
    }

    tb.loop();
}

static esp_err_t write_file(const char *path, char *data) {
    ESP_LOGI(TAG, "Opening file %s", path);
    FILE *f = fopen(path, "a+");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fprintf(f, data);
    fclose(f);
    ESP_LOGI(TAG, "Data appended");

    return ESP_OK;
}

static esp_err_t dump_data_to_tb(const char *path) {
    ESP_LOGI(TAG, "Reading file %s", path);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    char line[MAX_CHAR_SIZE];
    fgets(line, sizeof(line), f);
    while (!feof(f)) {
        ESP_LOGI(TAG, "Read from file: '%s'", line);
        tb.sendTelemetryJson(line);
        tb.loop();
        fgets(line, sizeof(line), f);
    }
    fclose(f);

    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }

    return ESP_OK;
}

extern "C" void app_main(void) {
    //Variables
    char flag_local_data = '0';
    bool provisioned = false;
    bool connected = false;
    int temp_in = 0;
    int hum_in = 0;
    int temp_out = 0;
    int noise = 0;
    int weight = 0;
    int channel = 1;
    int max_connections = 4;
    int attempts_recon_tb = 0;
    char token[] = "xxxxxxxxxxxxxxxxxxxx";
    char* ssid = "dummy_data";
    char* password = "dummy_data";
    char* ssid_prov = "Di-Core_Provisioning";
    char* password_prov = "provisioning1234!";
    esp_err_t wifi_err = ESP_OK;
    esp_err_t adc_err = ESP_OK;
    esp_err_t i2c_err = ESP_OK;
    esp_err_t sd_err = ESP_OK;
    time_t stamp;

    //SD Stuff
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        #ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
                .format_if_mount_failed = true,
        #else
                .format_if_mount_failed = false,
        #endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
                .max_files = 5,
                .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;

    
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    #ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
        slot_config.width = 4;
    #else
        slot_config.width = 1;
    #endif

    #ifdef CONFIG_SOC_SDMMC_USE_GPIO_MATRIX
        slot_config.clk = CONFIG_EXAMPLE_PIN_CLK;
        slot_config.cmd = CONFIG_EXAMPLE_PIN_CMD;
        slot_config.d0 = CONFIG_EXAMPLE_PIN_D0;
    #ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
        slot_config.d1 = CONFIG_EXAMPLE_PIN_D1;
        slot_config.d2 = CONFIG_EXAMPLE_PIN_D2;
        slot_config.d3 = CONFIG_EXAMPLE_PIN_D3;
    #endif  // CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
    #endif  // CONFIG_SOC_SDMMC_USE_GPIO_MATRIX

    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    const char *file_data = MOUNT_POINT"/local_data.txt";
    char data[MAX_CHAR_SIZE];


    init_nvs();

    //Set deep sleep to 30 secs
    esp_sleep_enable_timer_wakeup(27000000);

    //Initialize ADC
    adc_oneshot_unit_handle_t adc_handle = adc_manager_init_oneshot(ADC_UNIT_1);
    adc_err = adc_manager_cfg_channel_oneshot(adc_handle, ADC_CHANNEL_6, ADC_BITWIDTH_DEFAULT, ADC_ATTEN_DB_11);
    if (adc_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel 6! Err: %s", esp_err_to_name(adc_err));
    }
    adc_err = adc_manager_cfg_channel_oneshot(adc_handle, ADC_CHANNEL_4, ADC_BITWIDTH_DEFAULT, ADC_ATTEN_DB_11);
    if (adc_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel 4! Err: %s", esp_err_to_name(adc_err));
    }

    //Initialize other sensors
    i2c_err = init_PmodHYGRO();
    if (i2c_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize PmodHYGRO! Err: %s", esp_err_to_name(i2c_err));
    }
    i2c_err = PmodTMP3_read(&temp_out);
    if (i2c_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize PmodTMP3! Err: %s", esp_err_to_name(i2c_err));
    }

    //Read credentials from NVS
    read_string_from_nvs("ssid", ssid);
    read_string_from_nvs("password", password);
    read_string_from_nvs("token", token);
    read_string_from_nvs("flag_local_data", &flag_local_data);

    //Wifi setup
    wifi_err = connect_ap(ssid, password);
    if (wifi_err != ESP_OK) {
        ESP_LOGI(TAG, "Error (%s) connecting to AP!", esp_err_to_name(wifi_err));

        //Turn on flag_local_data
        write_string_to_nvs("flag_local_data", "1");
        flag_local_data = '1';

        //Setup AP for provisioning
        wifi_release();
        wifi_err = setup_ap(ssid_prov, password_prov, &channel, &max_connections);
        if (wifi_err != ESP_OK) {
            ESP_LOGE(TAG, "Error (%s) setting up AP!", esp_err_to_name(wifi_err));
            return void();
        }

        //TODO: Start provisioning server with callback

        //Setup SD card
        ESP_LOGI(TAG, "Initializing SD card");
        sd_err = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

        if (sd_err != ESP_OK) {
            if (sd_err == ESP_FAIL) {
                ESP_LOGE(TAG, "Failed to mount filesystem. If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
            } 
            
            else {
                ESP_LOGE(TAG, "Failed to initialize the card (%s). Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(sd_err));
            }
            return;
        }
        ESP_LOGI(TAG, "Filesystem mounted");

        for(;!provisioned;) {
            //Read sensor data
            i2c_err = PmodHYGRO_read(&temp_in, &hum_in);
            if (i2c_err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to read PmodHYGRO! Err: %s", esp_err_to_name(i2c_err));
            }
            i2c_err = PmodTMP3_read(&temp_out);
            if (i2c_err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to read PmodTMP3! Err: %s", esp_err_to_name(i2c_err));
            }
            adc_err = adc_manager_read_oneshot(adc_handle, ADC_CHANNEL_6, &noise);
            if (adc_err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to read ADC channel 6! Err: %s", esp_err_to_name(adc_err));
            }adc_err = adc_manager_read_oneshot(adc_handle, ADC_CHANNEL_4, &weight);
            if (adc_err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to read ADC channel 4! Err: %s", esp_err_to_name(adc_err));
            }

            //Save data to SD card
            time(&stamp);
            snprintf(data, MAX_CHAR_SIZE, "{'ts':%lld, 'values':{'%s':%d, '%s':%d, '%s':%d, '%s':%d, '%s':%d}}", stamp, "temperature_out", temp_out,
            "temperature_in", temp_in, "humidity_in", hum_in, "noise", noise, "weight", weight);
            sd_err = write_file(file_data, data);
            if (sd_err != ESP_OK) {
                ESP_LOGI(TAG, "Failed to write file : %s", file_data);
            }
            //Can't send to sleep to keep AP running
            vTaskDelay(30000 / portTICK_PERIOD_MS);
        }
        wifi_release();
        write_string_to_nvs("ssid", ssid);
        write_string_to_nvs("password", password);
        wifi_err = connect_ap(ssid, password);
    }
    
    if (!tb.connected()) {
        tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT);
    }

    
    if(flag_local_data == '1') {
        //Send all logged data to Thingsboard
        sd_err = dump_data_to_tb(file_data);
        if (sd_err != ESP_OK) {
            ESP_LOGI(TAG, "Failed to dump data: %s", file_data);
        }
        
        // All data sent, unmount partition and disable SDMMC peripheral
        esp_vfs_fat_sdcard_unmount(mount_point, card);
        ESP_LOGI(TAG, "Card unmounted");

        //Turn off flag_local_data
        write_string_to_nvs("flag_local_data", "0");
    }

    //We check for OTA updates
    OTA_process();

    //Execution loop
    for(;;) {
        // Every 30 seconds we read all of the sensors and dump the data
        esp_deep_sleep_start();
        i2c_err = PmodHYGRO_read(&temp_in, &hum_in);
        if (i2c_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read PmodHYGRO! Err: %s", esp_err_to_name(i2c_err));
        }
        i2c_err = PmodTMP3_read(&temp_out);
        if (i2c_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read PmodTMP3! Err: %s", esp_err_to_name(i2c_err));
        }
        adc_err = adc_manager_read_oneshot(adc_handle, ADC_CHANNEL_6, &noise);
        if (adc_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read ADC channel 6! Err: %s", esp_err_to_name(adc_err));
        }
        adc_err = adc_manager_read_oneshot(adc_handle, ADC_CHANNEL_4, &weight);
        if (adc_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read ADC channel 4! Err: %s", esp_err_to_name(adc_err));
        }

        tb.sendTelemetryData("temperature_in", temp_in);
        tb.sendTelemetryData("humidity_in", hum_in);
        tb.sendTelemetryData("temperature_out", temp_out);
        tb.sendTelemetryData("noise", noise);
        tb.sendTelemetryData("weight", weight);

        tb.loop();

        vTaskDelay(3000 / portTICK_PERIOD_MS); // Wait 3 seconds to avoid sleeping before sending data

        if(!connected) {
            esp_restart();
        }

        if (!tb.connected() && attempts_recon_tb < 3) {
            tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT);
            attempts_recon_tb++;
        }
        else if (!tb.connected()) {
            ESP_LOGE(TAG, "Failed to connect to Thingsboard! Err: %s", esp_err_to_name(ESP_ERR_TIMEOUT));
            esp_restart();
        }
        else {
            attempts_recon_tb = 0;
        }
    }
}
