cmake_minimum_required(VERSION 3.20)
project(vario_host_tests CXX)
set(CMAKE_CXX_STANDARD 17)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

add_executable(test_baro_math test_baro_math.cpp)
add_executable(test_ubx_parser test_ubx_parser.cpp ../src/drivers/ubx_parser.cpp)
add_executable(test_vario_dt test_vario_dt.cpp)
add_executable(test_lk8ex1_formatter test_lk8ex1_formatter.cpp ../src/middleware/lk8ex1_formatter.cpp)
add_executable(test_vario_engine test_vario_engine.cpp ../src/middleware/vario_engine.cpp)
add_executable(test_gnss_processor test_gnss_processor.cpp ../src/middleware/gnss_processor.cpp)
add_executable(test_battery_monitor test_battery_monitor.cpp ../src/middleware/battery_monitor.cpp mocks/mock_adc_hal.cpp)
add_executable(test_buzzer test_buzzer.cpp ../src/middleware/buzzer.cpp mocks/mock_pwm_hal.cpp)
add_executable(test_bmp390 test_bmp390.cpp ../src/drivers/bmp390.cpp mocks/mock_sensor_bus_hal.cpp)

enable_testing()
add_test(NAME baro_math        COMMAND test_baro_math)
add_test(NAME ubx_parser       COMMAND test_ubx_parser)
add_test(NAME vario_dt         COMMAND test_vario_dt)
add_test(NAME lk8ex1_formatter COMMAND test_lk8ex1_formatter)
add_test(NAME vario_engine     COMMAND test_vario_engine)
add_test(NAME gnss_processor   COMMAND test_gnss_processor)
add_test(NAME battery_monitor  COMMAND test_battery_monitor)
add_test(NAME buzzer           COMMAND test_buzzer)
add_test(NAME bmp390           COMMAND test_bmp390)
