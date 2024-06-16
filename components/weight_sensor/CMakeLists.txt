set(component_srcs)

idf_component_register(SRCS "hx711.c" "${component_srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES 
                       lib_helpers
                       esp_timer
                       driver)