/// @file vario_app.cpp
/// @brief Application orchestrator coordinating baro, GPS, BLE, audio, and battery.

#include "app/vario_app.hpp"
#include "hal/i2c_hal.hpp"
#include "hal/sensor_bus_hal.hpp"
#include "hal/pwm_hal.hpp"
#include "hal/adc_hal.hpp"
#include "hal/ble_hal.hpp"
#include "drivers/bmp390.hpp"
#include "drivers/sam_m10q.hpp"
#include "middleware/vario_engine.hpp"
#include "middleware/lk8ex1_formatter.hpp"
#include "middleware/ble_service.hpp"
#include "middleware/gnss_processor.hpp"
#include "middleware/buzzer.hpp"
#include "middleware/battery_monitor.hpp"
#include "common/config.hpp"
#include "common/types.hpp"

#include <zephyr/kernel.h>
#include <zephyr/drivers/watchdog.h>

namespace vario::app {

static struct k_timer s_loop_timer;
static struct k_msgq  s_gps_fix_msgq;
static char __aligned(4) s_gps_fix_msgq_buffer[4 * sizeof(GnssFix)];

static const struct device* const s_wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));
static int s_wdt_channel = -1;

static middleware::GnssProcessor  s_gnss_proc;
static middleware::Buzzer         s_buzzer;
static middleware::BatteryMonitor s_battery;
static VarioData                  s_last_vario = {};
static bool                       s_has_vario = false;
static int32_t                    s_last_qnh = config::kIsaSeaLevelPa;
static uint32_t                   s_overrun_count = 0;
static uint32_t                   s_last_vario_notify_ms = 0;
static uint32_t                   s_last_lk8_notify_ms = 0;
static uint32_t                   s_last_batt_ms = 0;

static void on_gps_fix_callback(const GnssFix* fix, void* user_data)
{
    ARG_UNUSED(user_data);
    if (fix != nullptr) {
        (void)k_msgq_put(&s_gps_fix_msgq, fix, K_NO_WAIT);
    }
}

static void wdt_start()
{
    if (!device_is_ready(s_wdt)) { return; }
    struct wdt_timeout_cfg cfg = {};
    cfg.flags = WDT_FLAG_RESET_SOC;
    cfg.window.min = 0U;
    cfg.window.max = config::kWdtTimeoutMs;
    s_wdt_channel = wdt_install_timeout(s_wdt, &cfg);
    if (s_wdt_channel < 0 || wdt_setup(s_wdt, WDT_OPT_PAUSE_HALTED_BY_DBG) != 0) {
        s_wdt_channel = -1;
    }
}

static void feed_watchdog()
{
    if (s_wdt_channel >= 0) {
        (void)wdt_feed(s_wdt, s_wdt_channel);
    }
}

static void play_boot_sound(bool degraded)
{
    s_buzzer.beep(1000, 80);
    k_msleep(160);
    s_buzzer.beep(1000, 80);
    k_msleep(160);
    if (degraded) {
        s_buzzer.beep(300, 400);
        k_msleep(450);
    }
}

static Error init_critical_hardware()
{
    if (hal::pwm_init() != Error::kOk) return Error::kPwmError;
    if (hal::i2c_init() != Error::kOk) return Error::kNotInitialized;
    if (hal::sensor_bus_init() != Error::kOk) return Error::kNotInitialized;
    if (bmp390::init() != Error::kOk) return Error::kNotInitialized;
    if (middleware::vario_engine_init() != Error::kOk) return Error::kNotInitialized;
    if (s_buzzer.init() != Error::kOk) return Error::kPwmError;
    if (s_gnss_proc.init() != Error::kOk) return Error::kNotInitialized;
    return Error::kOk;
}

static bool init_secondary_hardware()
{
    bool degraded = false;
    if (hal::adc_init() != Error::kOk || s_battery.init() != Error::kOk) {
        degraded = true;
    }
    if (hal::ble_init(nullptr) != Error::kOk ||
        middleware::ble_service_init() != Error::kOk ||
        hal::ble_start_advertising() != Error::kOk) {
        degraded = true;
    }
    if (gnss::sam_m10q_init(on_gps_fix_callback, nullptr) != Error::kOk) {
        degraded = true;
    }
    return degraded;
}

Error vario_app_init()
{
    k_timer_init(&s_loop_timer, nullptr, nullptr);
    k_msgq_init(&s_gps_fix_msgq, s_gps_fix_msgq_buffer, sizeof(GnssFix), 4);

    const Error crit_err = init_critical_hardware();
    if (crit_err != Error::kOk) {
        return crit_err;
    }

    const bool degraded = init_secondary_hardware();
    play_boot_sound(degraded);
    wdt_start();
    k_timer_start(&s_loop_timer, K_MSEC(config::kMainTickMs), K_MSEC(config::kMainTickMs));
    return Error::kOk;
}

static void service_baro(uint32_t tick)
{
    PressureData p {};
    const Error err = bmp390::read_pressure_temperature(&p);
    if (err != Error::kOk) return;

    VarioData vario_out {};
    (void)middleware::vario_engine_update(&p, &vario_out);
    s_gnss_proc.update_baro(p.pressure_cpa, vario_out.vario_cm_s, vario_out.alt_baro_cm);
    s_last_vario = vario_out;
    s_has_vario = true;

    if ((tick % config::kBuzzerUpdateTicks) == 0U) {
        s_buzzer.update(vario_out.vario_cm_s);
    }
}

static void process_gps_fix(const GnssFix& fix)
{
    s_gnss_proc.on_fix(&fix);
    const int32_t q = s_gnss_proc.calibrated_qnh_pa();
    if (q != s_last_qnh) {
        middleware::vario_engine_set_qnh(q);
        s_last_qnh = q;
    }
    if (hal::ble_is_connected()) {
        const auto& settings = middleware::ble_service_get_settings();
        if ((settings.protocol_mask & 0x02) != 0) {
            middleware::ble_service_notify_gps(
                fix, static_cast<uint8_t>(s_gnss_proc.flight_state()));
        }
    }
}

static void service_gps()
{
    GnssFix fix {};
    while (k_msgq_get(&s_gps_fix_msgq, &fix, K_NO_WAIT) == 0) {
        process_gps_fix(fix);
    }
    uint16_t status = s_gnss_proc.has_valid_fix() ? 0x0001 : 0x0000;
    status |= 0x0002;
    status |= static_cast<uint16_t>(static_cast<uint8_t>(s_gnss_proc.flight_state()) & 0x03) << 3;
    middleware::ble_service_update_status(status);
}

static void service_ble_settings()
{
    middleware::BleSettings new_settings {};
    if (middleware::ble_service_pop_settings(&new_settings)) {
        middleware::ble_service_apply_and_save_settings(new_settings);
    }
}

static void notify_vario_stream(uint32_t now, const middleware::BleSettings& settings)
{
    const uint32_t v_hz = settings.vario_rate_hz > 0 ? settings.vario_rate_hz : 10U;
    const uint32_t interval = 1000U / v_hz;
    if ((settings.protocol_mask & 0x01) && (now - s_last_vario_notify_ms >= interval)) {
        s_last_vario_notify_ms = now;
        middleware::ble_service_notify_vario(s_last_vario, s_last_vario.alt_baro_cm);
    }
}

static void notify_lk8ex1_stream(uint32_t now, const middleware::BleSettings& settings)
{
    const uint32_t l_hz = settings.lk8ex1_rate_hz > 0 ? settings.lk8ex1_rate_hz : 1U;
    const uint32_t interval = 1000U / l_hz;
    if ((settings.protocol_mask & 0x04) && (now - s_last_lk8_notify_ms >= interval)) {
        s_last_lk8_notify_ms = now;
        char sentence[config::kLk8ex1MaxLen] = {};
        uint8_t len = 0;
        if (middleware::lk8ex1_format(&s_last_vario, sentence, sizeof(sentence), &len) == Error::kOk) {
            middleware::ble_service_notify_lk8ex1(sentence, len);
        }
    }
}

static void service_ble_notify(uint32_t now)
{
    if (!hal::ble_is_connected() || !s_has_vario) return;
    const auto& settings = middleware::ble_service_get_settings();
    notify_vario_stream(now, settings);
    notify_lk8ex1_stream(now, settings);
}

static void service_battery(uint32_t now)
{
    if ((now - s_last_batt_ms) >= config::kBatterySampleIntervalMs) {
        s_last_batt_ms = now;
        (void)s_battery.update();
        if (hal::ble_is_connected()) {
            middleware::ble_service_notify_battery(s_battery.percent());
        }
    }
}

void vario_app_run()
{
    uint32_t tick = 0;
    for (;;) {
        if (k_timer_status_sync(&s_loop_timer) > 1U) {
            ++s_overrun_count;
        }
        feed_watchdog();
        const uint32_t now = k_uptime_get_32();
        ++tick;

        service_baro(tick);
        service_gps();
        service_ble_settings();
        service_ble_notify(now);
        service_battery(now);
    }
}

}  // namespace vario::app
