#include "esp_http_server.h"
#include "nvs_manager.h"
#include <string.h>

#include "web_server.h"

/* Wi-Fi credentials */
static char ssid[32] = {0};
static char password[64] = {0};

static esp_err_t root_handler(httpd_req_t *req)
{
    const char *html = "<html><body><form method=\"POST\">"
                        "<label>SSID: <input type=\"text\" name=\"ssid\"></label><br>"
                        "<label>Password: <input type=\"password\" name=\"password\"></label><br>"
                        "<input type=\"submit\" value=\"Connect\"></form></body></html>";
    
    //Get credentials when POST request is received
    if (req->method == HTTP_POST)
    {
        int total_len = req->content_len;
        int cur_len = 0;
        int received = 0;
        char *buf = malloc(total_len);
        if (!buf)
        {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        while (cur_len < total_len)
        {
            received = httpd_req_recv(req, buf + cur_len, total_len);
            if (received <= 0)
            {
                httpd_resp_send_500(req);
                free(buf);
                return ESP_FAIL;
            }
            cur_len += received;
        }
        buf[total_len] = '\0';

        //Parse SSID and password from POST request
        char param[32];
        if (httpd_query_key_value(buf, "ssid", param, sizeof(param)) == ESP_OK)
        {
            strcpy(ssid, param);
        }
        if (httpd_query_key_value(buf, "password", param, sizeof(param)) == ESP_OK)
        {
            strcpy(password, param);
        }
        free(buf);
    }

    //Save credentials to NVS
    init_nvs();
    write_string_to_nvs("ssid", ssid);
    write_string_to_nvs("password", password);

    //close server
    httpd_stop(req->handle);
    return ESP_OK;
}

/* HTTP server configuration */
static httpd_config_t httpd_config = HTTPD_DEFAULT_CONFIG();

static void start_httpd(void)
{
    httpd_handle_t server;
    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET | HTTP_POST,
        .handler = root_handler,
    };
    httpd_config.server_port = 80;
    httpd_start(&server, &httpd_config);
    httpd_register_uri_handler(server, &root_uri);
}
