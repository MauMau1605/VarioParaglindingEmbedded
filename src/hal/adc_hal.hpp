/// @file adc_hal.hpp
/// @brief Hardware abstraction layer for battery voltage measurement via Zephyr SAADC.

#ifndef VARIO_HAL_ADC_HAL_HPP
#define VARIO_HAL_ADC_HAL_HPP

#include <cstdint>
#include "common/error_codes.hpp"

namespace vario {
namespace hal {

/// @brief Initialize the SAADC channel AIN3 for high-impedance battery sensing.
/// @return Error::kOk on success.
Error adc_init();

/// @brief Sample battery voltage and return computed battery millivolts.
/// @param battery_mv Pointer to store measured voltage in millivolts.
/// @return Error::kOk on success.
Error adc_read_battery_mv(uint16_t* battery_mv);

}  // namespace hal
}  // namespace vario

#endif  // VARIO_HAL_ADC_HAL_HPP

