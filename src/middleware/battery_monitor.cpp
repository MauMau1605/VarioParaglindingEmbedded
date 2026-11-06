/// @file battery_monitor.cpp
/// @brief Piecewise linear battery state-of-charge estimator.

#include "middleware/battery_monitor.hpp"
#include "hal/adc_hal.hpp"

namespace vario::middleware {

struct BatteryPoint {
    uint16_t mv;
    uint8_t  pct;
};

static constexpr BatteryPoint kLut[] = {
    { 4200, 100 },
    { 4100, 90 },
    { 3900, 70 },
    { 3800, 50 },
    { 3700, 30 },
    { 3600, 15 },
    { 3500, 5 },
    { 3000, 0 }
};

Error BatteryMonitor::init()
{
    m_voltage_mv = 4000;
    m_percent = 80;
    m_initialized = false;
    return hal::adc_init();
}

uint8_t BatteryMonitor::voltage_to_percent(const uint16_t mv) const
{
    if (mv >= kLut[0].mv) return 100;
    if (mv <= kLut[7].mv) return 0;

    for (uint8_t i = 0; i < 7; ++i) {
        if (mv >= kLut[i + 1].mv) {
            const uint16_t v_span = kLut[i].mv - kLut[i + 1].mv;
            const uint16_t p_span = kLut[i].pct - kLut[i + 1].pct;
            const uint16_t v_off  = mv - kLut[i + 1].mv;
            return static_cast<uint8_t>(kLut[i + 1].pct + (v_off * p_span) / v_span);
        }
    }
    return 0;
}

Error BatteryMonitor::update()
{
    uint16_t read_mv = 0;
    const Error err = hal::adc_read_battery_mv(&read_mv);
    if (err != Error::kOk) return err;

    if (!m_initialized) {
        m_voltage_mv = read_mv;
        m_initialized = true;
    } else {
        m_voltage_mv = static_cast<uint16_t>(
            (3U * static_cast<uint32_t>(m_voltage_mv) + static_cast<uint32_t>(read_mv)) / 4U);
    }
    m_percent = voltage_to_percent(m_voltage_mv);
    return Error::kOk;
}

}  // namespace vario::middleware
