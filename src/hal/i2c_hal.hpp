/// @file i2c_hal.hpp
/// @brief Hardware abstraction layer for I2C communication via Zephyr I2C API.

#ifndef VARIO_HAL_I2C_HAL_HPP
#define VARIO_HAL_I2C_HAL_HPP

#include <cstdint>
#include "common/error_codes.hpp"

namespace vario {
namespace hal {

/// @brief Initialize the hardware I2C master controller.
/// @return Error::kOk on success, Error::kNotInitialized if device is not ready.
Error i2c_init();

/// @brief Read one or more consecutive registers from an I2C device.
/// @param device_addr 7-bit I2C slave address.
/// @param reg_addr    Register offset to begin reading from.
/// @param buffer      Destination memory buffer.
/// @param len         Number of bytes to read.
/// @return Error::kOk on success.
Error i2c_read_register(uint8_t device_addr,
                        uint8_t reg_addr,
                        uint8_t* buffer,
                        uint8_t len);

/// @brief Write one or more bytes starting at an I2C device register.
/// @param device_addr 7-bit I2C slave address.
/// @param reg_addr    Starting register address.
/// @param data        Source buffer containing bytes to write.
/// @param len         Number of bytes to write.
/// @return Error::kOk on success.
Error i2c_write_register(uint8_t device_addr,
                         uint8_t reg_addr,
                         const uint8_t* data,
                         uint8_t len);

}  // namespace hal
}  // namespace vario

#endif  // VARIO_HAL_I2C_HAL_HPP
