/// @file sensor_bus_hal.hpp
/// @brief Unified sensor bus interface (I2C or SPI) selected at compile-time.

#ifndef VARIOUSB_HAL_SENSOR_BUS_HAL_HPP
#define VARIOUSB_HAL_SENSOR_BUS_HAL_HPP

#include <cstdint>
#include "common/error_codes.hpp"

namespace vario {
namespace hal {

/// @brief Initialize the configured sensor bus (I2C or SPI).
/// @return Error::kOk on success.
Error sensor_bus_init();

/// @brief Read one or more bytes from a sensor register.
/// @param reg_addr Starting register address.
/// @param buffer   Caller-owned destination buffer.
/// @param len      Number of bytes to read.
/// @return Error::kOk on success.
Error sensor_bus_read_register(uint8_t reg_addr,
                               uint8_t* buffer,
                               uint8_t len);

/// @brief Write one or more bytes to a sensor register.
/// @param reg_addr Starting register address.
/// @param data     Data bytes to write.
/// @param len      Number of bytes to write.
/// @return Error::kOk on success.
Error sensor_bus_write_register(uint8_t reg_addr,
                                const uint8_t* data,
                                uint8_t len);

}  // namespace hal
}  // namespace vario

#endif  // VARIOUSB_HAL_SENSOR_BUS_HAL_HPP
