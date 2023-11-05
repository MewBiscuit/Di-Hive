set(component_srcs)

idf_component_register(SRCS "wifi_man_sta.c" "wifi_man_ap.c" "wifi_manager.c" "${component_srcs}"
                       PRIV_REQUIRES driver
                       INCLUDE_DIRS "include"
                       REQUIRES
                            esp_wifi
                            nvs_flash)