cmake_minimum_required(VERSION 3.13...3.27)

set(PICO_BOARD hanyazou_rp2350b_dip40)
set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/../include/boards)

include(../pico-sdk/pico_sdk_init.cmake)
project(rp2350b_dip40_test C CXX ASM)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

function(add_test_target target)
  add_executable(test_${target}
    ${target}.c
  )
  target_link_libraries(test_${target}
    pico_stdlib
    hardware_gpio
    hardware_spi
    hardware_i2c
  )
  target_compile_options(test_${target} PRIVATE
    -Og
    -g3
  )
  pico_enable_stdio_usb(test_${target} 1)
  pico_enable_stdio_uart(test_${target} 1)
  pico_add_extra_outputs(test_${target})
endfunction()

add_test_target(blink)
add_test_target(uart_echo)
add_test_target(spi_sd)
add_test_target(gpio)
add_test_target(i2c_scan)
add_test_target(i2c_rtc)
