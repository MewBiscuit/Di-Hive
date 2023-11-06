#ifndef WIFI_MAN_STA_H
#define WIFI_MAN_STA_H

#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "lwip/err.h"
#include "lwip/sys.h"

static esp_err_t wifi_init_sta();

/**
 * @brief Event handler for the Station mode.
 *
 * @param arg
 * @param event_base
 * @param event_id
 * @param event_data
 *
 * @return void
 */
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

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

#endif