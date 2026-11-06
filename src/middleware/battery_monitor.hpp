/// @file battery_monitor.hpp
/// @brief Battery state-of-charge estimator from ADC voltage readings.

#ifndef VARIO_MIDDLEWARE_BATTERY_MONITOR_HPP
#define VARIO_MIDDLEWARE_BATTERY_MONITOR_HPP

#include <cstdint>
#include "common/error_codes.hpp"

namespace vario::middleware {

class BatteryMonitor {
public:
    Error init();

    /// @brief Read ADC and compute new voltage and percentage estimate.
    /// @return Error::kOk on success.
    Error update();

    /// @brief Latest measured battery voltage in millivolts.
    uint16_t voltage_mv() const { return m_voltage_mv; }

    /// @brief Latest estimated State of Charge percentage (0-100%).
    uint8_t percent() const { return m_percent; }

private:
    uint16_t m_voltage_mv = 4000;
    uint8_t  m_percent = 80;
    bool     m_initialized = false;

    uint8_t voltage_to_percent(uint16_t mv) const;
};

}  // namespace vario::middleware

#endif  // VARIO_MIDDLEWARE_BATTERY_MONITOR_HPP
