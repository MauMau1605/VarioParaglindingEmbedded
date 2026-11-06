/// @file vario_engine.cpp
/// @brief Variometer engine implementation.

#include "middleware/vario_engine.hpp"
#include "common/config.hpp"
#include "common/baro_math.hpp"

#include <zephyr/kernel.h>

namespace vario::middleware {

static bool     s_initialized = false;
static bool     s_first_sample = true;
static int32_t  s_prev_pressure_cpa = 0;
static uint32_t s_prev_time_ms = 0;
static int32_t  s_filtered_vario_cm_s = 0;
static int32_t  s_qnh_pa = config::kIsaSeaLevelPa;
static int32_t  s_last_alt_cm = 0;

static int32_t apply_vario_filter(const int32_t raw_vario, const int32_t prev_filtered)
{
    const int32_t alpha = static_cast<int32_t>(config::kVarioFilterAlpha);
    const int32_t scale = static_cast<int32_t>(config::kVarioFilterScale);
    return static_cast<int32_t>((alpha * raw_vario + (scale - alpha) * prev_filtered) / scale);
}

Error vario_engine_init()
{
    s_first_sample = true;
    s_prev_pressure_cpa = 0;
    s_prev_time_ms = 0;
    s_filtered_vario_cm_s = 0;
    s_qnh_pa = config::kIsaSeaLevelPa;
    s_last_alt_cm = 0;
    s_initialized = true;
    return Error::kOk;
}

void vario_engine_set_qnh(int32_t qnh_pa)
{
    s_qnh_pa = (qnh_pa > 0) ? qnh_pa : config::kIsaSeaLevelPa;
}

static void handle_first_sample(const PressureData* pressure, const uint32_t now_ms, VarioData* out)
{
    s_prev_pressure_cpa = pressure->pressure_cpa;
    s_prev_time_ms = now_ms;
    s_first_sample = false;
    out->pressure_pa    = pressure->pressure_pa;
    out->vario_cm_s     = 0;
    out->alt_baro_cm    = baro_math::alt_cm_from_qnh(pressure->pressure_cpa, s_qnh_pa);
    out->temperature_dc = static_cast<int16_t>(pressure->temperature_cdeg / 10);
    s_last_alt_cm       = out->alt_baro_cm;
}

static void update_filter(const PressureData* pressure, const uint32_t now_ms)
{
    const uint32_t dt_ms = baro_math::quantize_baro_dt_ms(now_ms - s_prev_time_ms);
    const int32_t dp_cpa = pressure->pressure_cpa - s_prev_pressure_cpa;
    const int32_t dp_cpa_per_s = static_cast<int32_t>(
        (static_cast<int64_t>(dp_cpa) * 1000) / static_cast<int32_t>(dt_ms));
    const int32_t raw_vario = baro_math::vario_cm_s_from_dp(
        dp_cpa_per_s, pressure->pressure_pa, s_last_alt_cm);
    s_filtered_vario_cm_s = apply_vario_filter(raw_vario, s_filtered_vario_cm_s);
    s_prev_pressure_cpa   = pressure->pressure_cpa;
    s_prev_time_ms        = now_ms;
}

Error vario_engine_update(const PressureData* pressure, VarioData* out)
{
    if (!s_initialized) return Error::kNotInitialized;
    if (pressure == nullptr || out == nullptr) return Error::kInvalidData;

    const uint32_t now_ms = k_uptime_get_32();
    if (s_first_sample) {
        handle_first_sample(pressure, now_ms, out);
        return Error::kOk;
    }

    update_filter(pressure, now_ms);
    out->pressure_pa    = pressure->pressure_pa;
    out->vario_cm_s     = s_filtered_vario_cm_s;
    out->alt_baro_cm    = baro_math::alt_cm_from_qnh(pressure->pressure_cpa, s_qnh_pa);
    out->temperature_dc = static_cast<int16_t>(pressure->temperature_cdeg / 10);
    s_last_alt_cm       = out->alt_baro_cm;
    return Error::kOk;
}

}  // namespace vario::middleware
