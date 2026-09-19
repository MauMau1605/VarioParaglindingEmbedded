/// @file bmp390.hpp
/// @brief BMP390 pressure sensor driver public interface.
///        Handles initialization, configuration, and compensated data readout.

#ifndef VARIOUSB_DRIVERS_BMP390_HPP
#define VARIOUSB_DRIVERS_BMP390_HPP

#include "common/error_codes.hpp"
#include "common/types.hpp"

namespace vario {
namespace bmp390 {

/// @brief Initialize the BMP390 sensor.
///        Performs soft reset, verifies chip ID, reads calibration data,
///        configures oversampling, ODR, IIR filter, and starts normal mode.
/// @return Error::kOk on success.
Error init();

/// @brief Read compensated pressure and temperature.
///        Reads 6 raw data bytes and applies the Bosch compensation algorithm.
/// @param out Pointer to caller-owned PressureData struct to fill.
/// @return Error::kOk on success, error code on communication or data failure.
Error read_pressure_temperature(PressureData* out);

/// @brief Returns the raw chip ID detected during initialization.
/// @return 8-bit chip ID read from hardware (0x60 for BMP390, 0x50 for BMP388).
uint8_t get_detected_chip_id();

}  // namespace bmp390
}  // namespace vario

#endif  // VARIOUSB_DRIVERS_BMP390_HPP
