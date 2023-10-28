#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdint.h>
#include "esp_wifi.h"
#include "esp_check.h"

/**
 * @brief Initialize the WiFi manager.
 * 
 * @return esp_err_t
 */
esp_err_t wifi_init(void);


/**
 * @brief Connect to an access point with the given SSID and password.
 *
 * @param ssid The SSID of the access point to connect to.
 * @param password The password for the access point.
 *
 * @return esp_err_t
 */
esp_err_t connect_ap(const char* ssid, const char* password);

/**
 * @brief Set up an access point with the given SSID and password.
 *
 * @param ssid The SSID of the access point to set up.
 * @param password The password for the access point.
 *
 * @return esp_err_t
 */
esp_err_t setup_ap(const char* ssid, const char* password);

/**
 * @brief Disconnect from the current access point.
 *
 * @return esp_err_t
 */
esp_err_t disconnect_ap(void);

/**
 * @brief Release the wifi module
 *
 * @return esp_err_t
 */
esp_err_t wifi_release(void);


#endif /* WIFI_PROV_H */