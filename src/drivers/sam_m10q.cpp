/// @file sam_m10q.cpp
/// @brief u-blox SAM-M10Q GNSS driver using UBX binary protocol exclusively.

#include "drivers/sam_m10q.hpp"
#include "drivers/ubx_parser.hpp"
#include "hal/uart_hal.hpp"
#include "common/config.hpp"
#include <zephyr/kernel.h>

namespace vario::gnss {

enum class AckResult : uint8_t { kNone, kAck, kNak };

static FixCallback s_fix_cb = nullptr;
static void* s_user_data = nullptr;
static drivers::UbxParser s_parser;
static struct k_work s_rx_work;
static struct k_sem s_ack_sem;
static volatile AckResult s_ack_result = AckResult::kNone;

static bool fix_is_valid(const drivers::UbxNavPvt& pvt)
{
    return (pvt.fixType == 3)
        && ((pvt.flags & 0x01) != 0)
        && (pvt.numSV >= 6)
        && (pvt.vAcc < 5000);
}

static void fill_gnss_fix(const drivers::UbxNavPvt& pvt, GnssFix* fix)
{
    fix->valid             = fix_is_valid(pvt);
    fix->latitude_deg_e7   = pvt.lat;
    fix->longitude_deg_e7  = pvt.lon;
    fix->altitude_msl_mm   = pvt.hMSL;
    fix->ground_speed_mm_s = pvt.gSpeed;
    fix->heading_deg_e5    = pvt.headMot;
    fix->v_acc_mm          = pvt.vAcc;
    fix->satellites        = pvt.numSV;
    fix->fix_type          = pvt.fixType;
    fix->hour              = pvt.hour;
    fix->min               = pvt.min;
    fix->sec               = pvt.sec;
    fix->year              = pvt.year;
    fix->month             = pvt.month;
    fix->day               = pvt.day;
    fix->timestamp_ms      = k_uptime_get_32();
}

static void handle_ubx_event(drivers::UbxEvent evt)
{
    if (evt == drivers::UbxEvent::kNavPvt) {
        if (s_fix_cb != nullptr) {
            GnssFix fix = {};
            fill_gnss_fix(s_parser.nav_pvt(), &fix);
            s_fix_cb(&fix, s_user_data);
        }
    } else if (evt == drivers::UbxEvent::kAckAck) {
        if (s_parser.ack_class() == 0x06 && s_parser.ack_id() == 0x8A) {
            s_ack_result = AckResult::kAck;
            k_sem_give(&s_ack_sem);
        }
    } else if (evt == drivers::UbxEvent::kAckNak) {
        if (s_parser.ack_class() == 0x06 && s_parser.ack_id() == 0x8A) {
            s_ack_result = AckResult::kNak;
            k_sem_give(&s_ack_sem);
        }
    }
}

static void rx_work_handler(struct k_work* work)
{
    ARG_UNUSED(work);
    uint8_t buf[32];
    for (;;) {
        const uint16_t n = hal::uart_read(buf, sizeof(buf));
        if (n == 0) { break; }
        for (uint16_t i = 0; i < n; ++i) {
            handle_ubx_event(s_parser.feed(buf[i]));
        }
    }
}

static void on_rx_notify(void* user_data)
{
    ARG_UNUSED(user_data);
    k_work_submit(&s_rx_work);
}

static AckResult wait_ack(uint32_t timeout_ms)
{
    if (k_sem_take(&s_ack_sem, K_MSEC(timeout_ms)) == 0) {
        return s_ack_result;
    }
    return AckResult::kNone;
}

static Error send_valset_full()
{
    uint8_t buf[128];
    const uint16_t len = drivers::ubx_build_default_valset(buf, sizeof(buf));
    return hal::uart_send(buf, len);
}

static Error send_valset_baud(uint32_t baud)
{
    uint8_t buf[32];
    const uint16_t len = drivers::ubx_build_baud_valset(buf, sizeof(buf), baud);
    return hal::uart_send(buf, len);
}

static Error configure_at_current_baud()
{
    for (uint8_t i = 0; i < config::kGpsConfigSendsPerBaud; ++i) {
        k_sem_reset(&s_ack_sem);
        s_ack_result = AckResult::kNone;
        send_valset_full();
        if (wait_ack(config::kGpsAckTimeoutMs) == AckResult::kAck) {
            return Error::kOk;
        }
    }
    return Error::kTimeout;
}

static Error run_boot_handshake()
{
    for (uint8_t attempt = 0; attempt < config::kGpsInitMaxAttempts; ++attempt) {
        hal::uart_set_baud_rate(config::kGpsBaudRun);
        if (configure_at_current_baud() == Error::kOk) {
            return Error::kOk;
        }
        hal::uart_set_baud_rate(config::kGpsBaudCold);
        send_valset_baud(config::kGpsBaudRun);
        k_msleep(100);
        hal::uart_set_baud_rate(config::kGpsBaudRun);
        if (configure_at_current_baud() == Error::kOk) {
            return Error::kOk;
        }
        k_msleep(200);
    }
    return Error::kTimeout;
}

Error sam_m10q_init(FixCallback cb, void* user_data)
{
    s_fix_cb = cb;
    s_user_data = user_data;
    s_parser.reset();
    k_sem_init(&s_ack_sem, 0, 1);
    k_work_init(&s_rx_work, rx_work_handler);
    const Error err = hal::uart_init(on_rx_notify, nullptr);
    if (err != Error::kOk) {
        return err;
    }
    return run_boot_handshake();
}

}  // namespace vario::gnss
