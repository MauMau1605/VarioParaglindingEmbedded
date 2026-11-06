/// @file pwm_hal.hpp
/// @brief Hardware abstraction layer for differential piezo buzzer PWM control.

#ifndef VARIO_HAL_PWM_HAL_HPP
#define VARIO_HAL_PWM_HAL_HPP

#include <cstdint>
#include "common/error_codes.hpp"

namespace vario {
namespace hal {

/// @brief Initialize the differential PWM outputs on BUZ1 (P0.02) and BUZ2 (P0.03).
/// @return Error::kOk on success.
Error pwm_init();

/// @brief Configure differential frequency output across piezo terminals.
/// @param freq_hz Frequency in Hertz (0 turns off the tone safely).
/// @return Error::kOk on success.
Error pwm_set_frequency(uint32_t freq_hz);

/// @brief Force silence and guarantee 0V DC bias across both buzzer pins.
/// @return Error::kOk on success.
Error pwm_stop();

}  // namespace hal
}  // namespace vario

#endif  // VARIO_HAL_PWM_HAL_HPP

