#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_check.h"
#include "esp_wifi.h"
#include <stdint.h>

#include "wifi_man_ap.h"
#include "wifi_man_sta.h"

/**
 * @brief Release the wifi module
 *
 * @return esp_err_t
 */
esp_err_t wifi_release(void);

#endif