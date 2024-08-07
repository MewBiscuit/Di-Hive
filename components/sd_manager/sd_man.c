#include "sd_man.h"

sdmmc_card_t *card;

esp_err_t init_sd() {
    esp_err_t err;
    const char mount_point[] = "/sdcard";
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = 1000;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4092
    };

    err = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (err != ESP_OK) {
        ESP_LOGE(SD_TAG, "Failed to initialize SPI bus.");
        return err;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    err = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if (err != ESP_OK) {
        if (err == ESP_FAIL) {
            ESP_LOGE(SD_TAG, "Failed to mount filesystem. If you want the card to be formatted, set the CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } 
        
        else {
            ESP_LOGE(SD_TAG, "Failed to initialize the card (%s). Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(err));
        }
    }

    return err;
}

void read_sd_creds(bool* saved, char* ssid, char* password) {
    const char *credentials = "/sdcard/credentials.txt";
    struct stat st;
    FILE *f;

    //Check credentials
    if (stat(credentials, &st) == 0) { //If file exists
        *saved = true;
        f = fopen(credentials, "r");
        fgets(ssid, sizeof(ssid), f);
        fgets(password, sizeof(password), f);
        fclose(f);
    }

    else {
        *saved = false;
    }
}

void write_data(char* path, char* data) {
    FILE *f;
    struct stat st;

    f = fopen(path, "a");
    fprintf(f, data, card->cid.name);
    fclose(f);

    return;
}

void dump_data(char* path, char* topic, esp_mqtt_client_handle_t tb_client, int numlines, int offset, bool time_set) {
    FILE *f;
    const uint_fast8_t data_size = 512;
    char data[data_size];
    struct stat st;
    float sound = 0, temp_in = 0, temp_out = 0, hum_in = 0, hum_out = 0, weight = 0;
    time_t current_stamp = 0, read_stamp = 0;

    if(stat(path, &st) == 0) {//If file exists
        f = fopen(path, "r");
        if(time_set){
            while (fscanf(f, "{'ts':%lld, 'values':{'temperature_out':%f, 'temperature_in':%f, 'humidity_out':%f, 'humidity_in':%f, 'weight':%f, 'sound':%f}}", &read_stamp, &temp_out, &temp_in, &hum_out, &hum_in, &weight, &sound)) {
                snprintf(data, data_size, "{'ts':%lld, 'values':{'temperature_out':%f, 'temperature_in':%f, 'humidity_out':%f, 'humidity_in':%f, 'weight':%f, 'sound':%f}}", read_stamp, temp_out, temp_in, hum_out, hum_in, weight, sound);
                post_line(data, topic, tb_client);
                vTaskDelay(200 / portTICK_PERIOD_MS);
            }
        }

        else{
            while (fscanf(f, "{'ts':%lld, 'values':{'temperature_out':%f, 'temperature_in':%f, 'humidity_out':%f, 'humidity_in':%f, 'weight':%f, 'sound':%f}}", &read_stamp, &temp_out, &temp_in, &hum_out, &hum_in, &weight, &sound)) {
                read_stamp = current_stamp - (numlines * offset);
                numlines--;
                snprintf(data, data_size, "{'ts':%lld, 'values':{'temperature_out':%f, 'temperature_in':%f, 'humidity_out':%f, 'humidity_in':%f, 'weight':%f, 'sound':%f}}", read_stamp, temp_out, temp_in, hum_out, hum_in, weight, sound);
                post_line(data, topic, tb_client);
                vTaskDelay(200 / portTICK_PERIOD_MS);
            }
        }

        
        fclose(f);
        unlink(path);
    }

    return;
}