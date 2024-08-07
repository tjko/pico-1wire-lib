add_library(pico_1wire_lib INTERFACE)

target_include_directories(pico_1wire_lib INTERFACE
 ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(pico_1wire_lib INTERFACE
  hardware_gpio
)

target_sources(pico_1wire_lib INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/src/pico_1wire.c
)

