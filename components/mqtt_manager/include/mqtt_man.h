#ifndef MQTT_MAN_H
#define MQTT_MAN_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "cJSON.h"

static const char *TAG = "MQTT_MAN";

static void log_error_if_nonzero(const char *message, int error_code);

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

esp_mqtt_client_handle_t connect_mqtt_user_and_password(char *uri, int *port, char *username, char *password);

esp_mqtt_client_handle_t connect_mqtt_token(char *uri, int *port, char *token);

esp_err_t post_text_data(char *key, char *value, char* target_path, esp_mqtt_client_handle_t client);

esp_err_t post_int_data(char *key, int *value, char* target_path, esp_mqtt_client_handle_t client);

esp_err_t post_double_data(char *key, double *value, char* target_path, esp_mqtt_client_handle_t client);

#endif