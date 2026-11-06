/// @file ble_hal.cpp
/// @brief Zephyr BLE stack initialization, advertising, and link management.

#include "hal/ble_hal.hpp"
#include <errno.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>

#define BT_UUID_VARIO_SERVICE_VAL                                              \
  BT_UUID_128_ENCODE(0x564E4152, 0x494F, 0x0000, 0x0000, 0x000000000000ULL)
#define BT_UUID_NUS_SERVICE_VAL                                                \
  BT_UUID_128_ENCODE(0x6E400001, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9EULL)

namespace vario::hal {

static BleCallbacks s_callbacks = {};
static struct bt_conn *s_current_conn = nullptr;
static bool s_is_connected = false;

static const struct bt_data s_ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_VARIO_SERVICE_VAL),
};

static const struct bt_data s_sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
            sizeof(CONFIG_BT_DEVICE_NAME) - 1),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_SERVICE_VAL),
};

static void on_ble_connected(struct bt_conn *conn, uint8_t err) {
  if (err != 0) {
    return;
  }
  s_current_conn = bt_conn_ref(conn);
  s_is_connected = true;
  if (s_callbacks.on_connected != nullptr) {
    s_callbacks.on_connected(s_callbacks.user_data);
  }
}

static void on_ble_disconnected(struct bt_conn *conn, uint8_t reason) {
  ARG_UNUSED(conn);
  ARG_UNUSED(reason);
  if (s_current_conn != nullptr) {
    bt_conn_unref(s_current_conn);
    s_current_conn = nullptr;
  }
  s_is_connected = false;
  if (s_callbacks.on_disconnected != nullptr) {
    s_callbacks.on_disconnected(s_callbacks.user_data);
  }
}

static void on_conn_recycled(void) { (void)ble_start_advertising(); }

BT_CONN_CB_DEFINE(s_conn_callbacks) = {
    .connected = on_ble_connected,
    .disconnected = on_ble_disconnected,
    .recycled = on_conn_recycled,
};

Error ble_init(const BleCallbacks *callbacks) {
  if (callbacks != nullptr) {
    s_callbacks = *callbacks;
  }
  const int rc = bt_enable(nullptr);
  return (rc == 0) ? Error::kOk : Error::kBleError;
}

Error ble_start_advertising() {
  const struct bt_le_adv_param adv_param =
      *BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, 1600, 3200, nullptr);
  const int rc = bt_le_adv_start(&adv_param, s_ad, ARRAY_SIZE(s_ad), s_sd,
                                 ARRAY_SIZE(s_sd));
  return (rc == 0 || rc == -EALREADY) ? Error::kOk : Error::kBleError;
}

Error ble_stop_advertising() {
  const int rc = bt_le_adv_stop();
  return (rc == 0) ? Error::kOk : Error::kBleError;
}

bool ble_is_connected() { return s_is_connected; }

} // namespace vario::hal
