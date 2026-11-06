cmake_minimum_required(VERSION 3.20.0)
list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(vario_gps_ble LANGUAGES C CXX)

target_compile_options(app PRIVATE -Wall -Wextra -Werror -Wno-unused-parameter)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Source files — HAL
target_sources(app PRIVATE
    src/hal/i2c_hal.cpp
    src/hal/sensor_bus_hal.cpp
    src/hal/uart_hal.cpp
    src/hal/pwm_hal.cpp
    src/hal/adc_hal.cpp
    src/hal/ble_hal.cpp
)

# Source files — drivers
target_sources(app PRIVATE
    src/drivers/bmp390.cpp
    src/drivers/sam_m10q.cpp
    src/drivers/ubx_parser.cpp
)

# Source files — middleware
target_sources(app PRIVATE
    src/middleware/vario_engine.cpp
    src/middleware/lk8ex1_formatter.cpp
    src/middleware/ble_service.cpp
    src/middleware/ble_gatt.c
    src/middleware/gnss_processor.cpp
    src/middleware/buzzer.cpp
    src/middleware/battery_monitor.cpp
)

# Source files — app
target_sources(app PRIVATE
    src/app/vario_app.cpp
    src/main.cpp
)
