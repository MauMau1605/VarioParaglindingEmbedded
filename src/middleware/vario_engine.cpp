/// @file vario_engine.cpp
/// @brief Variometer engine implementation.
///        Converts pressure rate-of-change to vertical speed using:
///        vario ≈ -dP/dt × (RT)/(Mg) simplified to a barometric scale factor.
///        Uses fixed-point IIR filter to smooth the output.

#include "middleware/vario_engine.hpp"
#include "common/config.hpp"

#include <Arduino.h>  // millis()

namespace vario {
namespace middleware {

// ---------------------------------------------------------------------------
// Module state (static, no heap)
// ---------------------------------------------------------------------------
static bool     s_initialized = false;
static bool     s_first_sample = true;
static int32_t  s_prev_pressure_pa = 0;
static uint32_t s_prev_time_ms = 0;
static int32_t  s_filtered_vario_cm_s = 0;

/// Barometric scale factor (approximate):
/// Near sea level at 15°C: dz/dP ≈ -0.0832 m/Pa
/// Scaled to cm/s per Pa/s: factor ≈ -8 (simplified integer)
/// More precisely: -(RT)/(Mg) ≈ -8.3 cm per Pa, we use -8 for integer math.
static constexpr int32_t kBaroScaleFactor = -8;

Error vario_engine_init()
{
    s_first_sample = true;
    s_prev_pressure_pa = 0;
    s_prev_time_ms = 0;
    s_filtered_vario_cm_s = 0;
    s_initialized = true;
    return Error::kOk;
}

Error vario_engine_update(const PressureData* pressure, VarioData* out)
{
    if (!s_initialized) {
        return Error::kNotInitialized;
    }
    if (pressure == nullptr || out == nullptr) {
        return Error::kInvalidData;
    }

    const uint32_t now_ms = millis();

    if (s_first_sample) {
        // First sample: store reference, output zero vario
        s_prev_pressure_pa = pressure->pressure_pa;
        s_prev_time_ms = now_ms;
        s_first_sample = false;

        out->pressure_pa = pressure->pressure_pa;
        out->vario_cm_s = 0;
        out->temperature_dc = static_cast<int16_t>(
            pressure->temperature_cdeg / 10);
        return Error::kOk;
    }

    // Compute time delta (guard against zero)
    const uint32_t dt_ms = now_ms - s_prev_time_ms;
    if (dt_ms == 0) {
        out->pressure_pa = pressure->pressure_pa;
        out->vario_cm_s = s_filtered_vario_cm_s;
        out->temperature_dc = static_cast<int16_t>(
            pressure->temperature_cdeg / 10);
        return Error::kOk;
    }

    // Compute pressure rate of change: dP/dt in Pa/s (scaled ×1000 for precision)
    const int32_t dp = pressure->pressure_pa - s_prev_pressure_pa;
    const int32_t dp_dt_scaled = (dp * 1000) / static_cast<int32_t>(dt_ms);

    // Convert to vertical speed in cm/s
    const int32_t raw_vario = (dp_dt_scaled * kBaroScaleFactor) / 1000;

    // IIR low-pass filter: filtered = alpha * raw + (1 - alpha) * filtered
    // Using fixed-point: alpha = kVarioFilterAlpha / kVarioFilterScale
    s_filtered_vario_cm_s =
        (static_cast<int32_t>(config::kVarioFilterAlpha) * raw_vario +
         static_cast<int32_t>(config::kVarioFilterScale - config::kVarioFilterAlpha) *
             s_filtered_vario_cm_s) /
        static_cast<int32_t>(config::kVarioFilterScale);

    // Update state for next iteration
    s_prev_pressure_pa = pressure->pressure_pa;
    s_prev_time_ms = now_ms;

    // Fill output
    out->pressure_pa = pressure->pressure_pa;
    out->vario_cm_s = s_filtered_vario_cm_s;
    out->temperature_dc = static_cast<int16_t>(
        pressure->temperature_cdeg / 10);

    return Error::kOk;
}

}  // namespace middleware
}  // namespace vario
