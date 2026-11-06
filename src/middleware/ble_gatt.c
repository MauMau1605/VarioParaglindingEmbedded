/// @file ble_gatt.c
/// @brief Zephyr GATT service definitions, NUS support, and MTU-aware notifications.

#include "middleware/ble_gatt.h"
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <string.h>

#define BT_UUID_VARIO_SERVICE_VAL  BT_UUID_128_ENCODE(0x564E4152, 0x494F, 0x0000, 0x0000, 0x000000000000ULL)
#define BT_UUID_VARIO_DATA_VAL     BT_UUID_128_ENCODE(0x564E4152, 0x494F, 0x0000, 0x0000, 0x000000000001ULL)
#define BT_UUID_VARIO_GPS_VAL      BT_UUID_128_ENCODE(0x564E4152, 0x494F, 0x0000, 0x0000, 0x000000000002ULL)
#define BT_UUID_VARIO_BATT_VAL     BT_UUID_128_ENCODE(0x564E4152, 0x494F, 0x0000, 0x0000, 0x000000000003ULL)
#define BT_UUID_VARIO_STATUS_VAL   BT_UUID_128_ENCODE(0x564E4152, 0x494F, 0x0000, 0x0000, 0x000000000004ULL)
#define BT_UUID_VARIO_SETTINGS_VAL BT_UUID_128_ENCODE(0x564E4152, 0x494F, 0x0000, 0x0000, 0x000000000005ULL)

#define BT_UUID_NUS_SERVICE_VAL    BT_UUID_128_ENCODE(0x6E400001, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9EULL)
#define BT_UUID_NUS_RX_VAL         BT_UUID_128_ENCODE(0x6E400002, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9EULL)
#define BT_UUID_NUS_TX_VAL         BT_UUID_128_ENCODE(0x6E400003, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9EULL)

static struct bt_uuid_128 s_uuid_svc      = BT_UUID_INIT_128(BT_UUID_VARIO_SERVICE_VAL);
static struct bt_uuid_128 s_uuid_vario    = BT_UUID_INIT_128(BT_UUID_VARIO_DATA_VAL);
static struct bt_uuid_128 s_uuid_gps      = BT_UUID_INIT_128(BT_UUID_VARIO_GPS_VAL);
static struct bt_uuid_128 s_uuid_battery  = BT_UUID_INIT_128(BT_UUID_VARIO_BATT_VAL);
static struct bt_uuid_128 s_uuid_status   = BT_UUID_INIT_128(BT_UUID_VARIO_STATUS_VAL);
static struct bt_uuid_128 s_uuid_settings = BT_UUID_INIT_128(BT_UUID_VARIO_SETTINGS_VAL);

static struct bt_uuid_128 s_uuid_nus      = BT_UUID_INIT_128(BT_UUID_NUS_SERVICE_VAL);
static struct bt_uuid_128 s_uuid_nus_rx   = BT_UUID_INIT_128(BT_UUID_NUS_RX_VAL);
static struct bt_uuid_128 s_uuid_nus_tx   = BT_UUID_INIT_128(BT_UUID_NUS_TX_VAL);

static uint8_t  s_active_settings[4] = {0x03, 10, 1, 0};
static uint16_t s_device_status = 0;
static uint8_t  s_battery_percent = 100;
static uint16_t s_negotiated_mtu = 23;
static volatile uint32_t s_drop_count = 0;

K_MSGQ_DEFINE(s_settings_msgq, 4, 2, 1);

static ssize_t read_battery_cb(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                               void* buf, uint16_t len, uint16_t offset)
{
    ARG_UNUSED(conn);
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &s_battery_percent, sizeof(s_battery_percent));
}

static ssize_t read_status_cb(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                              void* buf, uint16_t len, uint16_t offset)
{
    ARG_UNUSED(conn);
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &s_device_status, sizeof(s_device_status));
}

static ssize_t read_settings_cb(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                void* buf, uint16_t len, uint16_t offset)
{
    ARG_UNUSED(conn);
    return bt_gatt_attr_read(conn, attr, buf, len, offset, s_active_settings, sizeof(s_active_settings));
}

static bool validate_rate(uint8_t rate)
{
    return (rate == 1 || rate == 2 || rate == 4 || rate == 5 || rate == 10);
}

static ssize_t write_settings_cb(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                 const void* buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    ARG_UNUSED(conn);
    ARG_UNUSED(attr);
    ARG_UNUSED(flags);
    if (offset != 0 || len != sizeof(s_active_settings)) {
        return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
    }
    const uint8_t* p = (const uint8_t*)buf;
    if (p[0] > 0x07 || !validate_rate(p[1]) || !validate_rate(p[2])) {
        return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
    }
    (void)k_msgq_put(&s_settings_msgq, buf, K_NO_WAIT);
    return len;
}

static ssize_t write_nus_rx_cb(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                               const void* buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    ARG_UNUSED(conn);
    ARG_UNUSED(attr);
    ARG_UNUSED(buf);
    ARG_UNUSED(offset);
    ARG_UNUSED(flags);
    return len;
}

BT_GATT_SERVICE_DEFINE(s_vario_svc,
    BT_GATT_PRIMARY_SERVICE(&s_uuid_svc),
    BT_GATT_CHARACTERISTIC(&s_uuid_vario.uuid, BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(&s_uuid_gps.uuid, BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(&s_uuid_battery.uuid, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ, read_battery_cb, NULL, &s_battery_percent),
    BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(&s_uuid_status.uuid, BT_GATT_CHRC_READ,
                           BT_GATT_PERM_READ, read_status_cb, NULL, &s_device_status),
    BT_GATT_CHARACTERISTIC(&s_uuid_settings.uuid, BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_settings_cb, write_settings_cb, s_active_settings)
);

BT_GATT_SERVICE_DEFINE(s_nus_svc,
    BT_GATT_PRIMARY_SERVICE(&s_uuid_nus),
    BT_GATT_CHARACTERISTIC(&s_uuid_nus_rx.uuid, BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_WRITE, NULL, write_nus_rx_cb, NULL),
    BT_GATT_CHARACTERISTIC(&s_uuid_nus_tx.uuid, BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
);

static void on_att_mtu_updated(struct bt_conn* conn, uint16_t tx, uint16_t rx)
{
    ARG_UNUSED(conn);
    ARG_UNUSED(rx);
    s_negotiated_mtu = tx;
}

static struct bt_gatt_cb s_gatt_cb = {
    .att_mtu_updated = on_att_mtu_updated,
};

void ble_gatt_init(void)
{
    bt_gatt_cb_register(&s_gatt_cb);
}

void ble_gatt_notify_vario(const void* data, uint16_t len)
{
    const uint16_t limit = (s_negotiated_mtu > 3) ? (s_negotiated_mtu - 3) : 20;
    if (len > limit) {
        ++s_drop_count;
        return;
    }
    const int rc = bt_gatt_notify(NULL, &s_vario_svc.attrs[2], data, len);
    if (rc != 0) {
        ++s_drop_count;
    }
}

void ble_gatt_notify_gps(const void* data, uint16_t len)
{
    const uint16_t limit = (s_negotiated_mtu > 3) ? (s_negotiated_mtu - 3) : 20;
    if (len > limit) {
        ++s_drop_count;
        return;
    }
    const int rc = bt_gatt_notify(NULL, &s_vario_svc.attrs[5], data, len);
    if (rc != 0) {
        ++s_drop_count;
    }
}

void ble_gatt_notify_battery(uint8_t percent)
{
    s_battery_percent = percent;
    const int rc = bt_gatt_notify(NULL, &s_vario_svc.attrs[8], &s_battery_percent, sizeof(s_battery_percent));
    if (rc != 0) {
        ++s_drop_count;
    }
}

void ble_gatt_notify_nus(const void* data, uint16_t len)
{
    if (data == NULL || len == 0) return;
    const uint8_t* p = (const uint8_t*)data;
    uint16_t remaining = len;
    const uint16_t max_chunk = (s_negotiated_mtu > 3) ? (s_negotiated_mtu - 3) : 20;

    while (remaining > 0) {
        const uint16_t chunk = (remaining < max_chunk) ? remaining : max_chunk;
        const int rc = bt_gatt_notify(NULL, &s_nus_svc.attrs[4], p, chunk);
        if (rc != 0) {
            ++s_drop_count;
            break;
        }
        p += chunk;
        remaining -= chunk;
    }
}

void ble_gatt_update_status(uint16_t status_flags)
{
    s_device_status = status_flags;
}

void ble_gatt_update_active_settings(const void* settings, size_t len)
{
    if (settings != NULL && len <= sizeof(s_active_settings)) {
        memcpy(s_active_settings, settings, len);
    }
}

bool ble_gatt_pop_settings(void* out_settings)
{
    return (k_msgq_get(&s_settings_msgq, out_settings, K_NO_WAIT) == 0);
}

uint32_t ble_gatt_get_drop_count(void)
{
    return s_drop_count;
}
