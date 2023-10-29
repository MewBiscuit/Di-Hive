set(component_srcs)

idf_component_register(SRCS "nvs_manager.c" "${component_srcs}"
                       PRIV_REQUIRES driver
                       INCLUDE_DIRS "include"
                       REQUIRES
                            nvs_flash)
