#ifndef WIFI_MAN_H
#define WIFI_MAN_H

#include <stdint.h>
#include <string.h>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_man.h"
#include "esp_system.h"


#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_softap.h>

static const char *WIFI_TAG = "wifi_manager";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define ESP_MAXIMUM_RETRY 10

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

/**
 * @brief Connect to an access point with the given SSID and password.
 *
 * @param ssid The SSID of the access point to connect to.
 * @param password The password for the access point.
 *
 * @return esp_err_t
 */
esp_err_t connect_ap(const char *ssid, const char *password);

/**
 * @brief Disconnect from the current access point.
 *
 * @return esp_err_t
 */
esp_err_t disconnect_ap(void);

/**
 * @brief Start the provisioning process.
 * 
 * @return esp_err_t
*/
esp_err_t start_provisioning();

/**
 * @brief Check if the device has been provisioned.
 * 
 * @return true 
 * @return false 
 */
bool is_provisioned();

/**
 * @brief Set up an access point with the given SSID and password.
 *
 * @param ssid The SSID of the access point to set up.
 * @param password The password for the access point.
 *
 * @return esp_err_t
 */
esp_err_t setup_ap(char *ssid, char *password, int *channel, int *max_connections);

esp_err_t wifi_init();

/**
 * @brief Release the wifi module
 *
 * @return esp_err_t
 */
esp_err_t wifi_release(void);

/**
 * @brief Event handler
 *
 * @param arg
 * @param event_base
 * @param event_id
 * @param event_data
 *
 * @return void
 */
void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

#endif