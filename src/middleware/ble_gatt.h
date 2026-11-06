/// @file ble_gatt.h
/// @brief C interface for Zephyr GATT service definitions and notifications.

#ifndef VARIO_MIDDLEWARE_BLE_GATT_H
#define VARIO_MIDDLEWARE_BLE_GATT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void ble_gatt_init(void);
void ble_gatt_notify_vario(const void* data, uint16_t len);
void ble_gatt_notify_gps(const void* data, uint16_t len);
void ble_gatt_notify_battery(uint8_t percent);
void ble_gatt_notify_nus(const void* data, uint16_t len);
void ble_gatt_update_status(uint16_t status_flags);
void ble_gatt_update_active_settings(const void* settings, size_t len);
bool ble_gatt_pop_settings(void* out_settings);
uint32_t ble_gatt_get_drop_count(void);

#ifdef __cplusplus
}
#endif

#endif  // VARIO_MIDDLEWARE_BLE_GATT_H
