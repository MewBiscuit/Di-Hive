#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdint.h>
#include "esp_wifi.h"
#include "esp_check.h"

#include "wifi_man_sta.h"
#include "wifi_man_ap.h"

/**
 * @brief Initialize the WiFi manager.
 * 
 * @return esp_err_t
 */
esp_err_t wifi_init(void);

/**
 * @brief Release the wifi module
 *
 * @return esp_err_t
 */
esp_err_t wifi_release(void);

#endif