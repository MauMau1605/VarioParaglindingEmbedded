/// @file vario_engine.cpp
/// @brief Variometer engine implementation.
///        Converts pressure rate-of-change to vertical speed using:
///        vario ≈ -dP/dt × (RT)/(Mg) ≈ −0.0832 m/Pa at sea-level ISA.
///        Uses fixed-point IIR filter to smooth the output.
///        Internally works in centiPascals (0.01 Pa) to avoid quantisation
///        noise that would otherwise alias 1 Pa steps at 50 Hz into
///        ±8 cm/s artefacts.

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
static int32_t  s_prev_pressure_cpa = 0;  // Previous pressure in centiPascals
static uint32_t s_prev_time_ms = 0;
static int32_t  s_filtered_vario_cm_s = 0;

/// Barometric scale factor for centiPascal inputs:
///   dz/dP ≈ −0.0832 m/Pa  = −8.32 cm/Pa = −0.0832 cm per cPa
/// Expressed as fraction: −832 / 10 000
/// Final formula: raw_vario_cm_s = (dp_cpa_per_s × −832) / 10 000
static constexpr int32_t kBaroScaleNum   = -832;   // numerator
static constexpr int32_t kBaroScaleDenom = 10000;  // denominator

static int32_t compute_raw_vario(const int32_t dp_cpa, const uint32_t dt_ms)
{
    const int32_t dp_cpa_per_s = (dp_cpa * 1000) / static_cast<int32_t>(dt_ms);
    return static_cast<int32_t>(
        (static_cast<int64_t>(dp_cpa_per_s) * kBaroScaleNum) / kBaroScaleDenom);
}

static int32_t apply_vario_filter(const int32_t raw_vario, const int32_t prev_filtered)
{
    return static_cast<int32_t>(
        (static_cast<int32_t>(config::kVarioFilterAlpha) * raw_vario +
         static_cast<int32_t>(config::kVarioFilterScale - config::kVarioFilterAlpha) *
             prev_filtered) /
        static_cast<int32_t>(config::kVarioFilterScale));
}

Error vario_engine_init()
{
    s_first_sample = true;
    s_prev_pressure_cpa = 0;
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
        s_prev_pressure_cpa = pressure->pressure_cpa;
        s_prev_time_ms = now_ms;
        s_first_sample = false;

        out->pressure_pa    = pressure->pressure_pa;
        out->vario_cm_s     = 0;
        out->temperature_dc = static_cast<int16_t>(pressure->temperature_cdeg / 10);
        return Error::kOk;
    }

    // Update vario on new time step
    const uint32_t dt_ms = now_ms - s_prev_time_ms;
    if (dt_ms > 0) {
        const int32_t dp_cpa = pressure->pressure_cpa - s_prev_pressure_cpa;
        const int32_t raw_vario = compute_raw_vario(dp_cpa, dt_ms);
        s_filtered_vario_cm_s = apply_vario_filter(raw_vario, s_filtered_vario_cm_s);

        s_prev_pressure_cpa = pressure->pressure_cpa;
        s_prev_time_ms      = now_ms;
    }

    // Fill output
    out->pressure_pa    = pressure->pressure_pa;
    out->vario_cm_s     = s_filtered_vario_cm_s;
    out->temperature_dc = static_cast<int16_t>(pressure->temperature_cdeg / 10);

    return Error::kOk;
}

}  // namespace middleware
}  // namespace vario
