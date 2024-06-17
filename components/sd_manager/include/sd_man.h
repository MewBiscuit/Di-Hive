#ifndef SD_MAN_H
#define SD_MAN_H

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "mqtt_man.h"
#include "esp_log.h"

#define PIN_NUM_MISO  GPIO_NUM_27
#define PIN_NUM_MOSI  GPIO_NUM_26
#define PIN_NUM_CLK   GPIO_NUM_25
#define PIN_NUM_CS    GPIO_NUM_33

#define SD_TAG "sd_manager"

esp_err_t init_sd();

void write_data(char* path, char* data);

void read_sd_creds(bool* saved, char* ssid, char* password);

void dump_data(char* path, char* topic, esp_mqtt_client_handle_t tb_client);

#endif