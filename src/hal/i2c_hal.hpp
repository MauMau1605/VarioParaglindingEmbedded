/// @file i2c_hal.hpp
/// @brief Hardware Abstraction Layer for I2C communication.
///        Wraps the Arduino Wire library to decouple upper layers from hardware.

#ifndef VARIOUSB_HAL_I2C_HAL_HPP
#define VARIOUSB_HAL_I2C_HAL_HPP

#include <cstdint>
#include "common/error_codes.hpp"

namespace vario {
namespace hal {

/// @brief Initialize the I2C peripheral.
/// @return Error::kOk on success.
Error i2c_init();

/// @brief Read one or more bytes from a device register.
/// @param device_addr 7-bit I2C device address.
/// @param reg_addr    Register address to read from.
/// @param buffer      Output buffer (caller-owned, must be >= len bytes).
/// @param len         Number of bytes to read.
/// @return Error::kOk on success, Error::kNack or Error::kTimeout on failure.
Error i2c_read_register(uint8_t device_addr,
                        uint8_t reg_addr,
                        uint8_t* buffer,
                        uint8_t len);

/// @brief Write one or more bytes to a device register.
/// @param device_addr 7-bit I2C device address.
/// @param reg_addr    Register address to write to.
/// @param data        Data to write (caller-owned).
/// @param len         Number of bytes to write.
/// @return Error::kOk on success, Error::kNack or Error::kWriteFailed on failure.
Error i2c_write_register(uint8_t device_addr,
                         uint8_t reg_addr,
                         const uint8_t* data,
                         uint8_t len);

}  // namespace hal
}  // namespace vario

#endif  // VARIOUSB_HAL_I2C_HAL_HPP
