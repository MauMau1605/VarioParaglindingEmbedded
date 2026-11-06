/// @file gnss_processor.hpp
/// @brief High-level GNSS processing, QNH auto-calibration, and flight state machine.

#ifndef VARIO_MIDDLEWARE_GNSS_PROCESSOR_HPP
#define VARIO_MIDDLEWARE_GNSS_PROCESSOR_HPP

#include <cstdint>
#include "common/error_codes.hpp"
#include "common/types.hpp"
#include "common/config.hpp"

namespace vario::middleware {

/// @brief Flight state machine states.
enum class FlightState : uint8_t {
    kWaitGpsFix      = 0,  ///< Waiting for initial 3D fix with high quality
    kPreTakeoffCalib = 1,  ///< On ground: rolling QNH calibration active
    kInFlight        = 2,  ///< In flight: QNH frozen
};

class GnssProcessor {
public:
    Error init();

    /// @brief Ingest a new 1 Hz GNSS fix from the SAM-M10Q driver.
    void on_fix(const GnssFix* fix);

    /// @brief Update latest barometric telemetry from the 50 Hz baro loop.
    void update_baro(int32_t pressure_cpa, int32_t vario_cm_s, int32_t alt_baro_cm = 0);

    /// @brief Current flight state machine state.
    FlightState flight_state() const { return m_state; }

    /// @brief Calibrated sea level reference pressure (QNH) in whole Pascals.
    int32_t calibrated_qnh_pa() const { return m_qnh_pa; }

    /// @brief Calculate barometric altitude in cm from current pressure and calibrated QNH.
    int32_t compute_alt_baro_cm(int32_t pressure_pa) const;

    /// @brief Check whether GNSS currently has a valid fix.
    bool has_valid_fix() const { return m_has_valid_fix; }

private:
    FlightState m_state = FlightState::kWaitGpsFix;
    bool        m_has_valid_fix = false;

    // Aligned rolling buffers for pre-takeoff calibration
    int32_t  m_ring_gps_alt_mm[config::kQnhWindowSamples] = {};
    int32_t  m_ring_pressure_cpa[config::kQnhWindowSamples] = {};
    uint8_t  m_ring_head = 0;
    uint8_t  m_ring_count = 0;
    int32_t  m_qnh_pa = config::kIsaSeaLevelPa;

    // Takeoff condition detection
    uint32_t m_takeoff_condition_start_ms = 0;
    bool     m_takeoff_condition_active = false;

    // Baro accumulation over 1-second GPS fix interval
    int64_t  m_baro_acc_cpa = 0;
    uint32_t m_baro_acc_n = 0;
    int32_t  m_last_pressure_cpa = config::kIsaSeaLevelPa * 100;
    int32_t  m_last_vario_cm_s = 0;

    void update_p0_calibration(int32_t alt_msl_mm, int32_t mean_press_cpa);
    void evaluate_takeoff(int32_t ground_speed_mm_s);
};

}  // namespace vario::middleware

#endif  // VARIO_MIDDLEWARE_GNSS_PROCESSOR_HPP
