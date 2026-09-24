cmake_minimum_required(VERSION 3.28...3.30)

project(kdr-running-text-obs VERSION 0.1.0)

set(PLUGIN_NAME "kdr-running-text-obs")
set(PLUGIN_AUTHOR "Abadi Jaya / KDR Multimedia")

option(ENABLE_FRONTEND_API "Use obs-frontend-api" OFF)

find_package(libobs REQUIRED)

add_library(kdr-running-text-obs MODULE
    src/plugin-main.c
    src/kdr-running-text.cpp
)

target_include_directories(kdr-running-text-obs PRIVATE src)
target_link_libraries(kdr-running-text-obs PRIVATE OBS::libobs)\n\nif(WIN32)\n    target_link_libraries(kdr-running-text-obs PRIVATE gdiplus)\nendif()

target_compile_definitions(kdr-running-text-obs PRIVATE
    PLUGIN_VERSION="0.1.0"
)

set_target_properties(kdr-running-text-obs PROPERTIES
    PREFIX ""
    OUTPUT_NAME "kdr-running-text-obs"
)

if(WIN32)
    set_target_properties(kdr-running-text-obs PROPERTIES SUFFIX ".dll")
endif()
