cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(remote_cat_toy C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_BOARD pico_w)
pico_sdk_init()

add_executable(remote_cat_toy
	callbacks.c
	storage.c
	wifi_saver.c
	dhcpserver.c
	main.c
)

pico_enable_stdio_usb(remote_cat_toy 1)
pico_enable_stdio_uart(remote_cat_toy 1)

pico_add_extra_outputs(remote_cat_toy)

target_include_directories(remote_cat_toy PRIVATE ${CMAKE_CURRENT_LIST_DIR} )
target_link_libraries(remote_cat_toy pico_cyw43_arch_lwip_threadsafe_background pico_stdlib hardware_flash )
