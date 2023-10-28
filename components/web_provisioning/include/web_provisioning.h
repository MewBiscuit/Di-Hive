#ifndef WEB_PROVISIONING_H
#define WEB_PROVISIONING_H

#include "esp_http_server.h"
#include <string.h>

/**
 * @brief HTTP server root handler. Writes captured credentials to nvs.
 * 
 * @param req The HTTP request.
 * 
 * @return esp_err_t
 */
esp_err_t root_handler(httpd_req_t *req);

/**
 * @brief Start the HTTP server.
 * 
 * @return void
 */
void start_httpd(void);


#endif