/// @file ble_service.hpp
/// @brief Custom GATT service for Vario, GNSS, and NUS data streaming over BLE.

#ifndef VARIO_MIDDLEWARE_BLE_SERVICE_HPP
#define VARIO_MIDDLEWARE_BLE_SERVICE_HPP

#include <cstdint>
#include "common/error_codes.hpp"
#include "common/types.hpp"

namespace vario::middleware {

struct __attribute__((packed)) BleVarioPayload {
    int32_t  vario_cm_s;
    int32_t  alt_baro_cm;
    uint32_t pressure_pa;
    int16_t  temperature_dc;
};
static_assert(sizeof(BleVarioPayload) == 14, "vario payload");

struct __attribute__((packed)) BleGpsPayload {
    int32_t  lat_e7;
    int32_t  lon_e7;
    int32_t  alt_msl_mm;
    int32_t  ground_speed_mm_s;
    uint32_t v_acc_mm;
    uint8_t  satellites;
    uint8_t  fix_type;
    uint16_t flight_state;
};
static_assert(sizeof(BleGpsPayload) == 24, "GPS payload");

struct __attribute__((packed)) BleSettings {
    uint8_t protocol_mask;   ///< bit 0: Vario binary, bit 1: GPS binary, bit 2: LK8EX1 over NUS
    uint8_t vario_rate_hz;   ///< Vario notify rate (1, 2, 4, 5, 10 Hz)
    uint8_t lk8ex1_rate_hz;  ///< LK8EX1 rate (1, 2, 4, 5, 10 Hz)
    uint8_t reserved;
};
static_assert(sizeof(BleSettings) == 4, "settings payload");

Error ble_service_init();
void  ble_service_notify_vario(const VarioData& data, int32_t alt_baro_cm);
void  ble_service_notify_gps(const GnssFix& fix, uint8_t flight_state);
void  ble_service_notify_battery(uint8_t percent);
void  ble_service_notify_lk8ex1(const char* sentence, uint8_t len);
void  ble_service_update_status(uint16_t status_flags);
const BleSettings& ble_service_get_settings();
bool  ble_service_pop_settings(BleSettings* out);
void  ble_service_apply_and_save_settings(const BleSettings& settings);

}  // namespace vario::middleware

#endif  // VARIO_MIDDLEWARE_BLE_SERVICE_HPP
