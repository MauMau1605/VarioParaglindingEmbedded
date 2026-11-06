/// @file ble_service.cpp
/// @brief Custom GATT service and NUS streaming with persisted settings.

#include "middleware/ble_service.hpp"
#include "middleware/ble_gatt.h"
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

namespace vario::middleware {

static BleSettings s_settings = {
    0x01 | 0x02, // protocol_mask
    10,          // vario_rate_hz
    1,           // lk8ex1_rate_hz
    0            // reserved
};

static int vario_settings_set(const char *name, size_t len,
                              settings_read_cb read_cb, void *cb_arg) {
  const char *next;
  if (settings_name_steq(name, "ble", &next) && !next) {
    if (len == sizeof(BleSettings)) {
      const ssize_t rc = read_cb(cb_arg, &s_settings, sizeof(s_settings));
      if (rc >= 0) {
        ble_gatt_update_active_settings(&s_settings, sizeof(s_settings));
        return 0;
      }
    }
  }
  return -ENOENT;
}

static struct settings_handler s_settings_handler = {};

Error ble_service_init() {
  ble_gatt_init();
  (void)settings_subsys_init();
  s_settings_handler.name = "vario";
  s_settings_handler.h_set = vario_settings_set;
  (void)settings_register(&s_settings_handler);
  (void)settings_load_subtree("vario");
  ble_gatt_update_active_settings(&s_settings, sizeof(s_settings));
  return Error::kOk;
}

void ble_service_notify_vario(const VarioData &data,
                              const int32_t alt_baro_cm) {
  const BleVarioPayload payload = {
      data.vario_cm_s,
      alt_baro_cm,
      static_cast<uint32_t>(data.pressure_pa),
      data.temperature_dc,
  };
  ble_gatt_notify_vario(&payload, sizeof(payload));
}

void ble_service_notify_gps(const GnssFix &fix, const uint8_t flight_state) {
  const BleGpsPayload payload = {
      fix.latitude_deg_e7,
      fix.longitude_deg_e7,
      fix.altitude_msl_mm,
      fix.ground_speed_mm_s,
      fix.v_acc_mm,
      fix.satellites,
      fix.fix_type,
      static_cast<uint16_t>(flight_state),
  };
  ble_gatt_notify_gps(&payload, sizeof(payload));
}

void ble_service_notify_battery(const uint8_t percent) {
  ble_gatt_notify_battery(percent);
}

void ble_service_notify_lk8ex1(const char *sentence, const uint8_t len) {
  if (sentence != nullptr && len > 0) {
    ble_gatt_notify_nus(sentence, len);
  }
}

void ble_service_update_status(const uint16_t status_flags) {
  ble_gatt_update_status(status_flags);
}

const BleSettings &ble_service_get_settings() { return s_settings; }

bool ble_service_pop_settings(BleSettings *out) {
  return ble_gatt_pop_settings(out);
}

void ble_service_apply_and_save_settings(const BleSettings &settings) {
  s_settings = settings;
  ble_gatt_update_active_settings(&s_settings, sizeof(s_settings));
  (void)settings_save_one("vario/ble", &s_settings, sizeof(s_settings));
}

} // namespace vario::middleware
