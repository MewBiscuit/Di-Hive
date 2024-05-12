set(component_srcs)

idf_component_register(SRCS "ota_man.c" "${component_srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES
                       esp_https_ota app_update)
