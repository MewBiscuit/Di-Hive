#include "http_prov.h"

esp_err_t post_handler(httpd_req_t *req) {
  char content[100];
  size_t recv_size = MIN(req->content_len, sizeof(content));

  int ret = httpd_req_recv(req, content, recv_size);
  if (ret <= 0) {
    if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
      httpd_resp_send_408(req);
    }
    return ESP_FAIL;
  }

  const char resp[] = "URI POST Response";
  httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

httpd_uri_t uri_post = {.uri = "/uri",
                        .method = HTTP_POST,
                        .handler = post_handler,
                        .user_ctx = NULL};

httpd_handle_t start_webserver(void) {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  httpd_handle_t server = NULL;

  if (httpd_start(&server, &config) == ESP_OK) {
    httpd_register_uri_handler(server, &uri_post);
  };

  return server;
}

void stop_webserver(httpd_handle_t server) {
  if (server) {
    httpd_stop(server);
  };
}