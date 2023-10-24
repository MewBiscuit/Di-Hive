set(component_srcs)

idf_component_register(SRCS "nvs_manager.c" "nvs_manager.h" "${component_srcs}"
                       PRIV_REQUIRES driver
                       INCLUDE_DIRS "."
                       REQUIRES
                            nvs_flash)
