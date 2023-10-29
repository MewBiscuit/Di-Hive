#ifndef WIFI_MANAGER_AP_H
#define WIFI_MANAGER_AP_H

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
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

/**
 * @brief Set up an access point with the given SSID and password.
 *
 * @param ssid The SSID of the access point to set up.
 * @param password The password for the access point.
 *
 * @return esp_err_t
 */
esp_err_t setup_ap(char* ssid, char* password);

#endif