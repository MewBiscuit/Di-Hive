set(component_srcs)

idf_component_register(SRCS "sensors_man.c" "${component_srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES
                       driver
                       weight_sensor)