set(component_srcs)

idf_component_register(SRCS "mqtt_man.c" "${component_srcs}"
                       PRIV_REQUIRES
                       INCLUDE_DIRS "include"
                       REQUIRES
                         esp_wifi
                         nvs_flash
                         mqtt
                         json)
