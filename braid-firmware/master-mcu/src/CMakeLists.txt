# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
message("All sources: ${app_sources}")

option(BOARD_LILYGO "Compile for the LilyGo SIM7000 devkit" OFF)
option(BOARD_DEVKITC "Compile for the Espressif ESP32-S3-C1 devkit" OFF)

if(BOARD_LILYGO)
    list(FILTER app_sources EXCLUDE REGEX "src/hal/devkitC/*")
    list(FILTER app_sources EXCLUDE REGEX "src/hal/devkitC.cpp")
    add_compile_definitions(BOARD_LILYGO)
endif()

if(BOARD_DEVKITC)
    list(FILTER app_sources EXCLUDE REGEX "src/hal/lilygo/*")
    list(FILTER app_sources EXCLUDE REGEX "src/hal/lilygo.cpp")
    add_compile_definitions(BOARD_DEVKITC)
endif()

message("After filter: ${app_sources}")

idf_component_register(
    SRCS ${app_sources}
)
