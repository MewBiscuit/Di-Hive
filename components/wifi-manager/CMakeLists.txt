set(component_srcs)

idf_component_register(SRCS "wifi_manager.c" "wifi_manager.h" "${component_srcs}"
                       PRIV_REQUIRES driver
                       INCLUDE_DIRS "."
                       REQUIRES
                            esp_wifi)
