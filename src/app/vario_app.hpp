/// @file vario_app.hpp
/// @brief Main application orchestrator for VarioUSB.
///        Coordinates all layers: HAL → Driver → Middleware → USB output.

#ifndef VARIOUSB_APP_VARIO_APP_HPP
#define VARIOUSB_APP_VARIO_APP_HPP

#include "common/error_codes.hpp"

namespace vario {
namespace app {

/// @brief Initialize all subsystems in dependency order.
///        Sequence: I2C HAL → USB Serial HAL → BMP390 driver → Vario engine.
/// @return Error::kOk if all subsystems initialized successfully.
Error vario_app_init();

/// @brief Main execution tick — non-blocking, called from Arduino loop().
///        Handles acquisition timing (50 Hz) and transmission timing (10 Hz).
///        Polls BMP390, updates vario engine, formats and sends LK8EX1 sentence.
Error vario_app_run();

}  // namespace app
}  // namespace vario

#endif  // VARIOUSB_APP_VARIO_APP_HPP
