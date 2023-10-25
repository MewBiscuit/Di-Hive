#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdint.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"


/**
 * @brief Connect to an access point with the given SSID and password.
 *
 * @param ssid The SSID of the access point to connect to.
 * @param password The password for the access point.
 *
 * @return 0 if the connection was successful, -1 otherwise.
 */
int wifi_connect_ap(const char* ssid, const char* password);

/**
 * @brief Set up an access point with the given SSID and password.
 *
 * @param ssid The SSID of the access point to set up.
 * @param password The password for the access point.
 *
 * @return 0 if the access point was set up successfully, -1 otherwise.
 */
int wifi_setup_ap(const char* ssid, const char* password);

#endif /* WIFI_PROV_H */
