/// @file error_codes.hpp
/// @brief Unified error codes for all layers of the VarioUSB project.

#ifndef VARIOUSB_COMMON_ERROR_CODES_HPP
#define VARIOUSB_COMMON_ERROR_CODES_HPP

#include <cstdint>

namespace vario {

/// @brief Status codes returned by all module functions.
///        kOk (0) indicates success; all other values indicate errors.
enum class Error : uint8_t {
    kOk = 0,          ///< Operation completed successfully
    kTimeout,          ///< Communication timeout (I2C, USB)
    kNack,             ///< I2C NACK received
    kBusy,             ///< Peripheral busy, retry later
    kInvalidData,      ///< Data read is corrupt or out of range
    kNotInitialized,   ///< Module used before initialization
    kInvalidChipId,    ///< BMP390 chip ID mismatch
    kWriteFailed,      ///< Write operation failed
    kReadFailed,       ///< Read operation failed
};

}  // namespace vario

#endif  // VARIOUSB_COMMON_ERROR_CODES_HPP
