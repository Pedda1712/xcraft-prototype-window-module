cmake_minimum_required(VERSION 3.16)
project(XCRAFT_WINDOW_MODULE)

set (XGAME_SOURCES
    ${CMAKE_SOURCE_DIR}/xcraft_window_module.c

)

add_library(xcraftwm STATIC ${XGAME_SOURCES})
target_compile_options(xcraftwm PUBLIC -O2 -Wall -Wextra)
