cmake_minimum_required(VERSION 3.20)
project(vario_host_tests CXX)
set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

add_executable(test_baro_math test_baro_math.cpp)
add_executable(test_ubx_parser test_ubx_parser.cpp ../src/drivers/ubx_parser.cpp)
add_executable(test_vario_dt test_vario_dt.cpp)

enable_testing()
add_test(NAME baro_math  COMMAND test_baro_math)
add_test(NAME ubx_parser COMMAND test_ubx_parser)
add_test(NAME vario_dt   COMMAND test_vario_dt)

