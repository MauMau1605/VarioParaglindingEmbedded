/// @file ble_hal.hpp
/// @brief Hardware abstraction layer for BLE stack initialization, advertising, and connection tracking.

#ifndef VARIO_HAL_BLE_HAL_HPP
#define VARIO_HAL_BLE_HAL_HPP

#include <cstdint>
#include "common/error_codes.hpp"

namespace vario {
namespace hal {

/// @brief Callbacks invoked on BLE link state changes.
struct BleCallbacks {
    void (*on_connected)(void* user_data);
    void (*on_disconnected)(void* user_data);
    void* user_data;
};

/// @brief Initialize the BLE subsystem and register connection callbacks.
/// @param callbacks Pointer to callback handlers (or nullptr).
/// @return Error::kOk on success.
Error ble_init(const BleCallbacks* callbacks);

/// @brief Start connectable BLE advertising with device name.
/// @return Error::kOk on success.
Error ble_start_advertising();

/// @brief Stop BLE advertising.
/// @return Error::kOk on success.
Error ble_stop_advertising();

/// @brief Check whether a central device is currently connected.
/// @return true if connected.
bool ble_is_connected();

}  // namespace hal
}  // namespace vario

#endif  // VARIO_HAL_BLE_HAL_HPP

