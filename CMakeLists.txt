add_library(pico-1wire-lib INTERFACE)

target_include_directories(pico-1wire-lib INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(pico-1wire-lib INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/src/pico_1wire.c
)

target_link_libraries(pico-1wire-lib INTERFACE
  hardware_gpio
)
