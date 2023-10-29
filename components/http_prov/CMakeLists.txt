set(component_srcs)

idf_component_register(SRCS "websocket_prov.c" "${component_srcs}"
                       PRIV_REQUIRES driver
                       INCLUDE_DIRS "include"
                       REQUIRES
                            )
