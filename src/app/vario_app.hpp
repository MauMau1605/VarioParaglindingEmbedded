/// @file vario_app.hpp
/// @brief Main application orchestrator for VarioGPS-BLE.

#ifndef VARIO_APP_VARIO_APP_HPP
#define VARIO_APP_VARIO_APP_HPP

#include "common/error_codes.hpp"

namespace vario::app {

/// @brief Initialize all hardware abstractions, drivers, middleware, and BLE.
/// @return Error::kOk on success, error code on fatal failure.
Error vario_app_init();

/// @brief Main deterministic execution loop (synchronized with 10 ms k_timer).
///        Never returns. Enters low-power WFE sleep when idle.
void vario_app_run();

}  // namespace app

#endif  // VARIO_APP_VARIO_APP_HPP
