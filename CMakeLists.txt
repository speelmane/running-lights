cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w CACHE STRING "Board type")

# build functions from pico sdk
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(running-lights C CXX ASM)

pico_sdk_init()

add_executable(${PROJECT_NAME})

# target_include_directories(${PROJECT_NAME} PUBLIC
#         ${CMAKE_CURRENT_LIST_DIR}/include/)

target_sources(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/main.c       
        )
# pull in common dependencies = library
target_link_libraries(${PROJECT_NAME} pico_stdlib pico_cyw43_arch_none)

# if (PICO_CYW43_SUPPORTED)
#     target_link_libraries(blink pico_cyw43_arch_none)
# endif()

# pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})
