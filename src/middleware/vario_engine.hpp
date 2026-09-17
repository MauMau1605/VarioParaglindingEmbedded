/// @file vario_engine.hpp
/// @brief Variometer engine — computes vertical speed from pressure data.
///        Uses a simple IIR filter on pressure derivative.

#ifndef VARIOUSB_MIDDLEWARE_VARIO_ENGINE_HPP
#define VARIOUSB_MIDDLEWARE_VARIO_ENGINE_HPP

#include "common/error_codes.hpp"
#include "common/types.hpp"

namespace vario {
namespace middleware {

/// @brief Initialize the vario engine internal state.
/// @return Error::kOk on success.
Error vario_engine_init();

/// @brief Update the vario engine with a new pressure sample.
///        Computes filtered vertical speed (cm/s) from pressure derivative.
/// @param pressure Input: latest compensated pressure data from BMP390.
/// @param out      Output: processed VarioData with vario, pressure, and temperature.
/// @return Error::kOk on success, Error::kNotInitialized if init was not called.
Error vario_engine_update(const PressureData* pressure, VarioData* out);

}  // namespace middleware
}  // namespace vario

#endif  // VARIOUSB_MIDDLEWARE_VARIO_ENGINE_HPP
