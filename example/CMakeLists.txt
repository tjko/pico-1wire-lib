# CMakeLists.txt

cmake_minimum_required(VERSION 3.18)

# Include Pico-SDK ($PICO_SDK_PATH must be set)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)


project(pico-1wire-example
  VERSION 1.0.0
  LANGUAGES C CXX ASM
  )
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)


set(CMAKE_BUILD_TYPE Debug)

message("---------------------------------")
message("          PICO_BOARD: ${PICO_BOARD}")
message("    CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message("---------------------------------")

pico_sdk_init()



add_subdirectory(../ pico-1wire-lib)


add_executable(pico-1wire-example
	example.c
)

pico_enable_stdio_usb(pico-1wire-example 1)
pico_enable_stdio_uart(pico-1wire-example 1)
pico_add_extra_outputs(pico-1wire-example)


target_link_libraries(pico-1wire-example PRIVATE
  pico_stdlib
  pico_stdio_uart
  pico_1wire_lib
)


target_compile_options(pico-1wire-example PRIVATE -Wall)


