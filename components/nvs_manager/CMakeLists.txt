set(component_srcs)

idf_component_register(SRCS "nvs_man.c" "${component_srcs}"
                       PRIV_REQUIRES
                       INCLUDE_DIRS "include"
                       REQUIRES
                            nvs_flash)
