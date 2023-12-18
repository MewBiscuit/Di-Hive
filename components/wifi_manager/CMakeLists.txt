set(component_srcs)

idf_component_register(SRCS "wifi_man.c" "${component_srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES
                            esp_wifi
                            nvs_manager
                            wifi_provisioning)