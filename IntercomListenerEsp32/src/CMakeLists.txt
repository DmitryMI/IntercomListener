FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${app_sources} Kconfig.projbuild)
# idf_component_register(SRCS "event.h" "led_indicator_task.cpp" "log_level.h" "main.cpp" "telegram.hpp" "timer.hpp" "wifi.h" "wifi.c")

# set(ulp_app_name ulp_main)
# set(ulp_s_sources ../ulp/ulp_main.S)
# set(ulp_exp_dep_srcs "main.cpp")

# ulp_embed_binary(${ulp_app_name} "${ulp_s_sources}" "${ulp_exp_dep_srcs}")