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

#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
#include "esp_efuse.h"
#endif

#define OTA_URL_SIZE 256
#define CONFIG_FIRMWARE_UPGRADE_URL "myurl.com"
#define CONFIG_OTA_RECV_TIMEOUT 10000

static const char* OTA_TAG = "ota_manager";
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

static esp_err_t _http_client_init_cb(esp_http_client_handle_t http_client);

/**
* @brief Check for any OTA updates, and if found, updates the Di-Core
* 
* @return esp_err_t
*/
esp_err_t check_updates();

#endif