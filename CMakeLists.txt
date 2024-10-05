cmake_minimum_required(VERSION 3.13)

include("$ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake")

project(pico_osc C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

include("pico-con/pico-con.cmake")

add_executable(pico_osc
	main.c
	measure.h
	measure.c
)

add_compile_definitions(
	$<$<CONFIG:Debug>:DEBUG>
)

pico_enable_stdio_usb(pico_osc 1)
pico_enable_stdio_uart(pico_osc 0)

pico_add_extra_outputs(pico_osc)

target_link_libraries(pico_osc pico_stdlib pico_con)
