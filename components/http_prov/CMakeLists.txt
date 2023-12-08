set(component_srcs)

idf_component_register(SRCS "http_prov.c" "${component_srcs}"
                       PRIV_REQUIRES
                       INCLUDE_DIRS "include"
                       REQUIRES
                            esp_http_server
                            nvs_flash
                            esp_netif)
