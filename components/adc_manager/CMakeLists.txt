set(component_srcs)

idf_component_register(SRCS "adc_man.c" "${component_srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES
                       esp_adc
                       driver)