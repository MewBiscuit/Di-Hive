set(component_srcs)

idf_component_register(SRCS "wifi_manager.c" "${component_srcs}"
                       PRIV_REQUIRES driver
                       INCLUDE_DIRS "include"
                       REQUIRES
                            esp_wifi)
