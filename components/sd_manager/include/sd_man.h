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
#include "esp_netif_sntp.h"

#define PIN_NUM_MISO  GPIO_NUM_2
#define PIN_NUM_MOSI  GPIO_NUM_15
#define PIN_NUM_CLK   GPIO_NUM_14
#define PIN_NUM_CS    GPIO_NUM_13

#define SD_TAG "sd_manager"

/**
 * @brief Initialize SD module and card
 * 
 * @return esp_err_t Error code
*/
esp_err_t init_sd();

/**
 * @brief Append specified data to specified file
 * 
 * @param path String containing path to file
 * @param data String containing data to be written
 * 
 * @return void
*/
void write_data(char* path, char* data);

/**
 * @brief Checks for saved credentials on SD card, and reads them if found
 * 
 * @param saved True if file exists, false otherwise
 * @param ssid String where SSID will be saved if found
 * @param password String where password will be saved if found
 * 
 * @return void
*/
void read_sd_creds(bool* saved, char* ssid, char* password);

/**
 * @brief Dumps data from specified file to specified mqtt client
 * 
 * @param path String containing path to file
 * @param topic String containing topic to which to publish data
 * @param tb_client Struct for mqtt client
 * @param numlines Number of lines written to file for internal timestamp sync
 * @param offset Number of offset for internal timestamp synchronization
 * @param time_set Flag for whether timestamp is correct
 * 
 * @return void
*/
void dump_data(char* path, char* topic, esp_mqtt_client_handle_t tb_client, int numlines, int offset, bool time_set);
#endif