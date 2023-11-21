#ifndef HTTP_PROV_H
#define HTTP_PROV_H

#include <esp_http_server.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

#define HTTP_QUERY_KEY_MAX_LEN (64)
#define HTTPD_401 "401 UNAUTHORIZED"

static const char *TAG = "http_prov";



#endif