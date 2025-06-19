cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(remote_cat_toy C CXX ASM)
project(blink C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_BOARD pico_w)
pico_sdk_init()

add_executable(blink
	picow_blink.c
)
add_executable(remote_cat_toy
	callbacks.c
	storage.c
	wifi_saver.c
	dhcpserver.c
	main.c
)

pico_enable_stdio_usb(remote_cat_toy 1)
pico_enable_stdio_uart(remote_cat_toy 1)
pico_enable_stdio_usb(blink 1)
pico_enable_stdio_uart(blink 1)

pico_add_extra_outputs(remote_cat_toy)
pico_add_extra_outputs(blink)
target_include_directories(blink PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(blink pico_cyw43_arch_lwip_threadsafe_background pico_stdlib)
target_include_directories(remote_cat_toy PRIVATE ${CMAKE_CURRENT_LIST_DIR} )
target_link_libraries(remote_cat_toy pico_cyw43_arch_lwip_threadsafe_background pico_stdlib hardware_flash )
