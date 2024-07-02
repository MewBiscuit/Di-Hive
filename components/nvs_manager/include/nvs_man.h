#ifndef NVS_MANAGER
#define NVS_MANAGER

#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"

static const char* NVS_TAG = "nvs_manager";

/**
    * @brief Initialize the default NVS partition.
    * 
    * @return Error code
*/
esp_err_t init_nvs();

/**
    * @brief Read a string from NVS.
    * 
    * @param key The key to read from NVS.
    * @param value The value to read from NVS.
    * 
    * @return Error code
*/
esp_err_t read_string_from_nvs(char *key, char *value);

/**
    * @brief Write a string to NVS.
    * 
    * @param key The key to write to NVS.
    * @param value The value to write to NVS.
    * 
    * @return Error code
*/
esp_err_t write_string_to_nvs(char *key, char *value);
#endif