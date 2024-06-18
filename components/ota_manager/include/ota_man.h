#ifndef OTA_MANAGER
#define OTA_MANAGER

#include <string.h>
#include <inttypes.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_crt_bundle.h"

#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
#include "esp_efuse.h"
#endif

#define OTA_URL_SIZE 256
#define CONFIG_FIRMWARE_UPGRADE_URL "http://192.168.43.229:8080/api/v1/4IdMOqwMILd426ebULui/firmware?title=DiCoreFW&version=0.5.0.0"
#define CONFIG_OTA_RECV_TIMEOUT 10000

static const char* OTA_TAG = "ota_manager";

/**
* @brief Check for any OTA updates, and if found, updates the Di-Core
* 
* @return esp_err_t
*/
esp_err_t check_updates();

#endif