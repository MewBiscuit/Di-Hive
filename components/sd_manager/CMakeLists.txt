set(component_srcs)

idf_component_register(SRCS "sd_man.c" "${component_srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES 
                       fatfs
                       mqtt_manager)