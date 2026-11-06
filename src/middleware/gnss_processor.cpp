/// @file gnss_processor.cpp
/// @brief GNSS high-level flight state machine and integer QNH calibration.

#include "middleware/gnss_processor.hpp"
#include "common/baro_math.hpp"
#include <zephyr/kernel.h>

namespace vario::middleware {

Error GnssProcessor::init()
{
    m_state = FlightState::kWaitGpsFix;
    m_has_valid_fix = false;
    m_ring_head = 0;
    m_ring_count = 0;
    m_qnh_pa = config::kIsaSeaLevelPa;
    m_takeoff_condition_start_ms = 0;
    m_takeoff_condition_active = false;
    m_baro_acc_cpa = 0;
    m_baro_acc_n = 0;
    m_last_pressure_cpa = config::kIsaSeaLevelPa * 100;
    m_last_vario_cm_s = 0;
    return Error::kOk;
}

int32_t GnssProcessor::compute_alt_baro_cm(const int32_t pressure_pa) const
{
    return baro_math::alt_cm_from_qnh(pressure_pa * 100, m_qnh_pa);
}

void GnssProcessor::update_baro(const int32_t pressure_cpa, const int32_t vario_cm_s, const int32_t alt_baro_cm)
{
    (void)alt_baro_cm;
    m_baro_acc_cpa += pressure_cpa;
    ++m_baro_acc_n;
    m_last_pressure_cpa = pressure_cpa;
    m_last_vario_cm_s   = vario_cm_s;
}

void GnssProcessor::evaluate_takeoff(const int32_t ground_speed_mm_s)
{
    const int32_t abs_vz = (m_last_vario_cm_s >= 0) ? m_last_vario_cm_s : -m_last_vario_cm_s;
    const bool vz_only = (abs_vz > config::kTakeoffVzCmS);
    const bool vz_speed = (ground_speed_mm_s > config::kTakeoffSpeedMmS) &&
                          (abs_vz > config::kTakeoffVzWithSpeedCmS);
    const bool condition = vz_only || vz_speed;
    const uint32_t now = k_uptime_get_32();

    if (!condition) {
        m_takeoff_condition_active = false;
        return;
    }
    if (!m_takeoff_condition_active) {
        m_takeoff_condition_active = true;
        m_takeoff_condition_start_ms = now;
    } else if ((now - m_takeoff_condition_start_ms) >= config::kTakeoffHoldMs) {
        m_state = FlightState::kInFlight;
    }
}

void GnssProcessor::update_p0_calibration(const int32_t alt_msl_mm, const int32_t mean_press_cpa)
{
    m_ring_gps_alt_mm[m_ring_head] = alt_msl_mm;
    m_ring_pressure_cpa[m_ring_head] = mean_press_cpa;
    m_ring_head = (m_ring_head + 1U) % config::kQnhWindowSamples;
    if (m_ring_count < config::kQnhWindowSamples) {
        ++m_ring_count;
    }
    if (m_ring_count >= config::kQnhMinSamples) {
        int64_t sum_alt = 0;
        int64_t sum_press = 0;
        for (uint8_t i = 0; i < m_ring_count; ++i) {
            sum_alt += m_ring_gps_alt_mm[i];
            sum_press += m_ring_pressure_cpa[i];
        }
        const int32_t avg_alt_mm = static_cast<int32_t>(sum_alt / m_ring_count);
        const int32_t avg_press_cpa = static_cast<int32_t>(sum_press / m_ring_count);
        m_qnh_pa = baro_math::qnh_pa_from_alt(avg_press_cpa, avg_alt_mm);
    }
}

void GnssProcessor::on_fix(const GnssFix* fix)
{
    if (fix == nullptr || !fix->valid) {
        m_has_valid_fix = false;
        if (m_state == FlightState::kPreTakeoffCalib) {
            m_state = FlightState::kWaitGpsFix;
        }
        m_ring_count = 0;
        m_ring_head = 0;
        m_takeoff_condition_active = false;
        return;
    }

    m_has_valid_fix = true;
    if (m_state == FlightState::kWaitGpsFix) {
        m_state = FlightState::kPreTakeoffCalib;
    }

    const int32_t mean_press_cpa = (m_baro_acc_n > 0U)
        ? static_cast<int32_t>(m_baro_acc_cpa / m_baro_acc_n)
        : m_last_pressure_cpa;
    m_baro_acc_cpa = 0;
    m_baro_acc_n = 0;

    if (m_state == FlightState::kPreTakeoffCalib) {
        evaluate_takeoff(fix->ground_speed_mm_s);
        if (!m_takeoff_condition_active &&
            (fix->ground_speed_mm_s < config::kQnhStationarySpeedMmS)) {
            update_p0_calibration(fix->altitude_msl_mm, mean_press_cpa);
        }
    }
}

}  // namespace vario::middleware
