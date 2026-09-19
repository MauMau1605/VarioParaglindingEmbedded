/// @file spi_hal.hpp
/// @brief Hardware Abstraction Layer for SPI communication.
///        Wraps Arduino SPI library for Bosch BMP390 communication.

#ifndef VARIOUSB_HAL_SPI_HAL_HPP
#define VARIOUSB_HAL_SPI_HAL_HPP

#include <cstdint>
#include "common/error_codes.hpp"

namespace vario {
namespace hal {

/// @brief Initialize the SPI bus and configure the Chip Select pin.
/// @param cs_pin Arduino pin used for Chip Select (CS).
/// @return Error::kOk on success.
Error spi_init(uint8_t cs_pin);

/// @brief Read registers over 4-wire SPI with Bosch dummy-byte support.
/// @param cs_pin   Chip select pin.
/// @param reg_addr Starting register address.
/// @param buffer   Output buffer for read bytes.
/// @param len      Number of data bytes to read.
/// @return Error::kOk on success, Error::kInvalidData on null buffer.
Error spi_read_register(uint8_t cs_pin,
                        uint8_t reg_addr,
                        uint8_t* buffer,
                        uint8_t len);

/// @brief Write registers over 4-wire SPI.
/// @param cs_pin   Chip select pin.
/// @param reg_addr Starting register address.
/// @param data     Data bytes to write.
/// @param len      Number of data bytes to write.
/// @return Error::kOk on success, Error::kInvalidData on null data.
Error spi_write_register(uint8_t cs_pin,
                         uint8_t reg_addr,
                         const uint8_t* data,
                         uint8_t len);

}  // namespace hal
}  // namespace vario

#endif  // VARIOUSB_HAL_SPI_HAL_HPP
