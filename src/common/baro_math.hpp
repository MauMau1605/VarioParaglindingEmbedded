#ifndef VARIO_COMMON_BARO_MATH_HPP
#define VARIO_COMMON_BARO_MATH_HPP

#include <cstdint>
#include "common/config.hpp"

namespace vario::baro_math {

inline constexpr int32_t  kLutMinAltM = -500;
inline constexpr int32_t  kLutStepM   = 100;
inline constexpr uint32_t kLutSize    = 46U;
inline constexpr int32_t  kMinAltCm   = kLutMinAltM * 100;
inline constexpr int32_t  kMaxAltCm   = (kLutMinAltM + static_cast<int32_t>(kLutSize - 1U) * kLutStepM) * 100;

inline constexpr uint32_t kQnhScaleLut[kLutSize] = {
    61785U,  62514U,  63254U,  64004U,  64765U,  65536U,   // -500 .. 0 m (index 5 = 0 m)
    66318U,  67112U,  67917U,  68733U,  69561U,  70401U,   // +100 .. +600
    71253U,  72118U,  72995U,  73884U,  74787U,  75703U,   // +700 .. +1200
    76632U,  77574U,  78531U,  79501U,  80486U,  81486U,   // +1300 .. +1800
    82500U,  83529U,  84574U,  85634U,  86711U,  87803U,   // +1900 .. +2400
    88911U,  90037U,  91179U,  92339U,  93516U,  94711U,   // +2500 .. +3000
    95925U,  97157U,  98407U,  99678U, 100967U, 102277U,   // +3100 .. +3600
   103606U, 104957U, 106328U, 107721U                       // +3700 .. +4000
};

inline int32_t clamp_alt_cm(int32_t alt_cm) {
    return (alt_cm < kMinAltCm) ? kMinAltCm : ((alt_cm > kMaxAltCm) ? kMaxAltCm : alt_cm);
}

// scale = P0 / P (Q16.16) for a given altitude, linear interpolation in the LUT
inline uint32_t scale_from_alt_cm(int32_t alt_cm) {
    const uint32_t off = static_cast<uint32_t>(clamp_alt_cm(alt_cm) - kMinAltCm);
    uint32_t idx = off / 10000U;                 // 100 m = 10000 cm
    uint32_t rem = off % 10000U;
    if (idx >= kLutSize - 1U) {
        idx = kLutSize - 2U;
        rem = 10000U;
    }
    const uint32_t s0 = kQnhScaleLut[idx];
    const uint32_t s1 = kQnhScaleLut[idx + 1U];
    return s0 + ((s1 - s0) * rem) / 10000U;
}

// Forward: P0 (whole Pa) from measured pressure (cPa) and GPS altitude (mm MSL)
inline int32_t qnh_pa_from_alt(int32_t pressure_cpa, int32_t alt_mm) {
    if (pressure_cpa <= 0) {
        return config::kIsaSeaLevelPa;
    }
    const uint32_t scale  = scale_from_alt_cm(alt_mm / 10);
    const uint64_t p0_cpa = (static_cast<uint64_t>(pressure_cpa) * scale) >> 16;   // 64-bit: mandatory
    return static_cast<int32_t>((p0_cpa + 50U) / 100U);
}

// Inverse: altitude (cm) from scale, binary search + interpolation
inline int32_t alt_cm_from_scale(uint32_t scale) {
    if (scale <= kQnhScaleLut[0]) {
        return kMinAltCm;
    }
    if (scale >= kQnhScaleLut[kLutSize - 1U]) {
        return kMaxAltCm;
    }
    uint32_t lo = 0U;
    uint32_t hi = kLutSize - 1U;
    while (lo + 1U < hi) {
        const uint32_t mid = (lo + hi) / 2U;
        if (kQnhScaleLut[mid] <= scale) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    const uint32_t s0 = kQnhScaleLut[lo];
    const uint32_t s1 = kQnhScaleLut[hi];
    const int32_t base_cm = kMinAltCm + static_cast<int32_t>(lo) * kLutStepM * 100;
    return base_cm + static_cast<int32_t>(((scale - s0) * 10000U) / (s1 - s0));
}

inline int32_t alt_cm_from_qnh(int32_t pressure_cpa, int32_t qnh_pa) {
    if (pressure_cpa <= 0 || qnh_pa <= 0) {
        return 0;
    }
    const uint64_t q = static_cast<uint64_t>(qnh_pa) * 100ULL;
    return alt_cm_from_scale(static_cast<uint32_t>((q << 16) / static_cast<uint32_t>(pressure_cpa)));
}

// Vertical speed (cm/s) from filtered dp/dt (cPa/s). ISA temperature from altitude.
//   dh = -(R/g)·T · dP/P   with R/g = 29.271 m/K   =>   K = 29.271·T  (≈ 8434 at 288.15 K)
//   dp in cPa, P in Pa, result in cm:  vz_cm_s = -dp_cpa_per_s · K / P_pa
inline int32_t vario_cm_s_from_dp(int32_t dp_cpa_per_s, int32_t pressure_pa, int32_t alt_cm) {
    if (pressure_pa <= 0) {
        return 0;
    }
    const int32_t t_dk = 2882 - (65 * clamp_alt_cm(alt_cm)) / 100000;   // ISA T in 0.1 K (−6.5 K/km)
    const int32_t k    = (29271 * t_dk) / 10000;                         // ≈ 8435 at sea level
    return static_cast<int32_t>((-static_cast<int64_t>(dp_cpa_per_s) * k) / pressure_pa);
}

// DRDY dt quantizer: maps elapsed time between samples to multiples of kBaroSamplePeriodMs (min 1 period)
inline uint32_t quantize_baro_dt_ms(uint32_t elapsed_ms) {
    uint32_t n = (elapsed_ms + config::kBaroSamplePeriodMs / 2U) / config::kBaroSamplePeriodMs;
    if (n < 1U) {
        n = 1U;
    }
    return n * config::kBaroSamplePeriodMs;
}

}  // namespace vario::baro_math

#endif  // VARIO_COMMON_BARO_MATH_HPP
