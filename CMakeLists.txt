cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)

set(APP_NAME rpi-pico-dns-sinkhole)

set(APP_VERSION "0.1")

set(PICO_BOARD pico_w)

include(pico_sdk_import.cmake)

project(${APP_NAME} C CXX ASM)

pico_sdk_init()

add_executable(${APP_NAME} main.c source/flash.c source/block.c)

pico_set_program_name(${APP_NAME} "${APP_NAME}")

pico_set_program_version(${APP_NAME} "${APP_VERSION}")

pico_enable_stdio_uart(${APP_NAME} 0)

pico_enable_stdio_usb(${APP_NAME} 1)

target_include_directories(${APP_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(${APP_NAME} pico_stdlib pico_cyw43_arch_lwip_threadsafe_background)

pico_add_linker_script_override_path(${APP_NAME} ${CMAKE_CURRENT_LIST_DIR}/linker FILES pico_flash_region.ld)

pico_add_extra_outputs(${APP_NAME})