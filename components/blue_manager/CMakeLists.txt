set(component_srcs)

idf_component_register(SRCS "${component_srcs}"
                       PRIV_REQUIRES driver
                       INCLUDE_DIRS "include"
                       REQUIRES
                            nvs-manager)
