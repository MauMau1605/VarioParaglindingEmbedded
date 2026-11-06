/// @file sensor_bus_hal.hpp
/// @brief Unified sensor bus facade for the BMP390 barometric sensor.

#ifndef VARIO_HAL_SENSOR_BUS_HAL_HPP
#define VARIO_HAL_SENSOR_BUS_HAL_HPP

#include <cstdint>
#include "common/error_codes.hpp"

namespace vario {
namespace hal {

/// @brief Initialize the physical communication bus for sensor access.
/// @return Error::kOk on success.
Error sensor_bus_init();

/// @brief Read registers from the BMP390 sensor.
/// @param reg_addr Starting register address.
/// @param buffer   Caller-allocated destination buffer.
/// @param len      Number of bytes to read.
/// @return Error::kOk on success.
Error sensor_bus_read_register(uint8_t reg_addr,
                               uint8_t* buffer,
                               uint8_t len);

/// @brief Write registers to the BMP390 sensor.
/// @param reg_addr Starting register address.
/// @param data     Data bytes to write.
/// @param len      Number of bytes to write.
/// @return Error::kOk on success.
Error sensor_bus_write_register(uint8_t reg_addr,
                                const uint8_t* data,
                                uint8_t len);

}  // namespace hal
}  // namespace vario

#endif  // VARIO_HAL_SENSOR_BUS_HAL_HPP
