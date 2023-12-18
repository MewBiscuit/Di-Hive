#ifndef WIFI_MANAGER_AP_H
#define WIFI_MANAGER_AP_H

#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_man.h"

#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_softap.h>

static const char *AP_TAG = "wifi_man_ap";

static esp_err_t wifi_init_ap();

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
 * @brief event_handler for the access point.
 *
 * @param arg
 * @param event_base
 * @param event_id
 * @param event_data
 *
 * @return void
 */
void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

/**
 * @brief Set up an access point with the given SSID and password.
 *
 * @param ssid The SSID of the access point to set up.
 * @param password The password for the access point.
 *
 * @return esp_err_t
 */
esp_err_t setup_ap(char *ssid, char *password, int *channel, int *max_connections);

#endif