/// @file types.hpp
/// @brief Shared data types used across all layers of VarioUSB.

#ifndef VARIOUSB_COMMON_TYPES_HPP
#define VARIOUSB_COMMON_TYPES_HPP

#include <cstdint>

namespace vario {

/// @brief Raw compensated pressure and temperature from the BMP390 driver.
///        Used as the output of the driver layer and input to the middleware.
struct PressureData {
    int32_t pressure_pa;       ///< Compensated pressure in Pascals (e.g., 101325 Pa)
    int32_t temperature_cdeg;  ///< Compensated temperature in centidegrees Celsius (e.g., 2150 = 21.50 °C)
};

/// @brief Processed variometer data ready for LK8EX1 formatting.
///        Produced by the vario engine in the middleware layer.
struct VarioData {
    int32_t pressure_pa;    ///< Pressure in Pascals
    int32_t vario_cm_s;     ///< Vertical speed in cm/s (positive = climb, negative = sink)
    int16_t temperature_dc; ///< Temperature in decidegrees Celsius (e.g., 215 = 21.5 °C)
};

}  // namespace vario

#endif  // VARIOUSB_COMMON_TYPES_HPP
