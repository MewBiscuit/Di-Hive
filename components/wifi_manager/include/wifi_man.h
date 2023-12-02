#ifndef WIFI_MAN_H
#define WIFI_MAN_H

#include <stdint.h>

#include "esp_check.h"
#include "esp_wifi.h"

#include "wifi_man_ap.h"
#include "wifi_man_sta.h"

static const char *WIFI_TAG = "wifi_manager";

/**
 * @brief Release the wifi module
 *
 * @return esp_err_t
 */
esp_err_t wifi_release(void);

#endif