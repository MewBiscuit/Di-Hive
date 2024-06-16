#include "sd_man.h"

sdmmc_card_t *card;

esp_err_t init_sd() {
    esp_err_t err;
    const char mount_point[] = "/sdcard";
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5
    };

    slot_config.width = 4;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    err = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if(err != ESP_OK) {
        ESP_LOGE(SD_TAG, "Could not initialize SD_Card module");
        return err;
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
        f = fopen(credentials, 'r');
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

    f = fopen(*path, "a"); //TODO: The pointer is why paralelization doesn't work most likely, will check out once sequential is done
    fprintf(f, *data, card->cid.name);
    fclose(f);

    return;
}

void dump_data(char* path, char* topic, esp_mqtt_client_handle_t tb_client) {
    FILE *f;
    const uint_fast8_t data_size = 256;
    char data[data_size];
    struct stat st;

    if (stat(*path, &st) == 0) {//If file exists
        f = fopen(*path, "r");
        while (fgets(data, sizeof(data), f)) {
            post_line(data, topic, tb_client);
        }
        
        fclose(f);
        unlink(f);
    }

    return;
}