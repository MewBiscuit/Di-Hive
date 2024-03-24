set(component_srcs)

idf_component_register(SRCS "mic_man.c" "${component_srcs}"
                       INCLUDE_DIRS "include")