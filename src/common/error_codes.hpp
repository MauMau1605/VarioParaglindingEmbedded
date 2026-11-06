/// @file error_codes.hpp
/// @brief Unified error codes for all layers of the VarioGPS-BLE project.

#ifndef VARIO_COMMON_ERROR_CODES_HPP
#define VARIO_COMMON_ERROR_CODES_HPP

#include <cstdint>

namespace vario {

/// @brief Status codes returned by all module functions.
///        kOk (0) indicates success; all other values indicate errors.
enum class Error : uint8_t {
    kOk             = 0,   ///< Operation completed successfully
    kTimeout        = 1,   ///< Communication timeout
    kNack           = 2,   ///< I2C NACK received
    kBusy           = 3,   ///< Peripheral busy, retry later
    kInvalidData    = 4,   ///< Data corrupt or out of range
    kNotInitialized = 5,   ///< Module used before initialization
    kInvalidChipId  = 6,   ///< Sensor chip ID mismatch
    kWriteFailed    = 7,   ///< Write operation failed
    kReadFailed     = 8,   ///< Read operation failed
    kUartError      = 9,   ///< UART communication error
    kParseError     = 10,  ///< UBX parsing or checksum error
    kNoFix          = 11,  ///< GNSS has no valid 3D fix
    kBleError       = 12,  ///< BLE stack or notify error
    kAdcError       = 13,  ///< ADC reading error
    kPwmError       = 14,  ///< PWM/buzzer error
};

}  // namespace vario

#endif  // VARIO_COMMON_ERROR_CODES_HPP
