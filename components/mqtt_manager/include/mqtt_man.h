#ifndef MQTT_MAN_H
#define MQTT_MAN_H

#include "esp_event.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "cJSON.h"

static const char *MQTT_TAG = "MQTT_MAN";

/**
 * @brief Connect to MQTT broker with username and password
 * 
 * @param uri URI of the MQTT broker
 * @param port Port of the MQTT broker
 * @param username Username of the MQTT broker
 * @param password Password of the MQTT broker
 * 
 * @return esp_mqtt_client_handle_t Handle of the MQTT client
*/
esp_mqtt_client_handle_t connect_mqtt_user_and_password(char *uri, int *port, char *username, char *password);
 
 /**
  * @brief Connect to MQTT broker with token
  * 
  * @param uri URI of the MQTT broker
  * @param port Port of the MQTT broker
  * @param token Token of the MQTT broker
  * 
  * @return esp_mqtt_client_handle_t Handle of the MQTT client
 */
esp_mqtt_client_handle_t connect_mqtt_token(char *uri, int *port, char *token);

/**
 * @brief Post text data to MQTT broker
 * 
 * @param key Key of the data
 * @param value Value of the data
 * @param target_path Target path of the data
 * @param client Handle of the MQTT client
 * 
 * @return esp_err_t Error code
*/
esp_err_t post_text_data(char *key, char *value, char *target_path, esp_mqtt_client_handle_t client);

/**
 * @brief Post numerical data to MQTT broker
 * 
 * @param key Key of the data
 * @param value Value of the data
 * @param target_path Target path of the data
 * @param client Handle of the MQTT client
 * 
 * @return esp_err_t Error code
*/
esp_err_t post_numerical_data(char *key, float *value, char *target_path, esp_mqtt_client_handle_t client);

/**
 * @brief Post line of data to MQTT broker
 * 
 * @param line Data to be posted
 * @param target_path Target path of the data
 * @param client Handle of the MQTT client
 * 
 * @return esp_err_t Error code
*/
esp_err_t post_line(char* line, char* target_path, esp_mqtt_client_handle_t client) ;

#endif