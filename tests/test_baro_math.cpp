#include "check.hpp"
#include "common/baro_math.hpp"
#include <cmath>
#include <iostream>

using namespace vario::baro_math;

int main() {
    std::cout << "Running test_baro_math..." << std::endl;

    // 1. LUT vs libm
    constexpr double H = 44330.0;
    for (uint32_t i = 0; i < kLutSize; ++i) {
        double h = kLutMinAltM + static_cast<double>(i) * kLutStepM;
        double ref = std::round(65536.0 * std::pow(1.0 - h / H, -5.255));
        CHECK_NEAR(kQnhScaleLut[i], ref, 1.0);
    }

    // 2. Accuracy vs physics: h in -500..4000 step 50
    for (int32_t h = -500; h <= 4000; h += 50) {
        double P = 101325.0 * std::pow(1.0 - static_cast<double>(h) / H, 5.255);
        int32_t cpa = static_cast<int32_t>(std::round(P * 100.0));
        int32_t alt_cm = alt_cm_from_qnh(cpa, 101325);
        double calc_h = alt_cm / 100.0;
        CHECK_NEAR(calc_h, static_cast<double>(h), 0.5);
    }

    // 3. Round trip for altitudes {-300, 0, 500, 1234, 2000, 3000, 3900}
    const int32_t test_alts[] = {-300, 0, 500, 1234, 2000, 3000, 3900};
    for (int32_t alt_m : test_alts) {
        double P = 101325.0 * std::pow(1.0 - static_cast<double>(alt_m + 400) / H, 5.255);
        int32_t cpa = static_cast<int32_t>(std::round(P * 100.0));
        int32_t qnh = qnh_pa_from_alt(cpa, alt_m * 1000);
        int32_t calc_alt_cm = alt_cm_from_qnh(cpa, qnh);
        double calc_alt_m = calc_alt_cm / 100.0;
        CHECK_NEAR(calc_alt_m, static_cast<double>(alt_m), 0.25);
    }

    // 4. Clamping & invalid inputs
    CHECK(clamp_alt_cm(-60000) == kMinAltCm);
    CHECK(clamp_alt_cm(500000) == kMaxAltCm);
    CHECK(alt_cm_from_qnh(-10, 101325) == 0);
    CHECK(alt_cm_from_qnh(100000, 0) == 0);
    CHECK(qnh_pa_from_alt(0, 1000) == vario::config::kIsaSeaLevelPa);
    CHECK(qnh_pa_from_alt(-500, 1000) == vario::config::kIsaSeaLevelPa);
    CHECK(vario_cm_s_from_dp(100, 0, 0) == 0);

    // 5. Vario scale
    // P=101325, dp=-1200 cPa/s, alt=0 -> 99 +/- 1
    int32_t vz_sl = vario_cm_s_from_dp(-1200, 101325, 0);
    CHECK_NEAR(vz_sl, 99, 1);

    // P=74675, dp=-1200, alt=250000 cm -> within +/- 1.5 of float reference
    double t_ref = 288.15 - 0.0065 * 2500.0;
    double vz_ref = 12.0 * (29.271 * t_ref) / 74675.0 * 100.0;
    int32_t vz_alt = vario_cm_s_from_dp(-1200, 74675, 250000);
    CHECK_NEAR(vz_alt, vz_ref, 1.5);

    // Sign: dp > 0 -> negative Vz
    int32_t vz_down = vario_cm_s_from_dp(1200, 101325, 0);
    CHECK(vz_down < 0);

    // 6. No 32-bit overflow
    // pressure_cpa = 11000000 (1100 hPa), alt = 4000 m (4000000 mm)
    int32_t p_large = 11000000;
    int32_t qnh_large = qnh_pa_from_alt(p_large, 4000000);
    CHECK(qnh_large > 0);
    uint32_t sc = scale_from_alt_cm(400000);
    uint64_t ref_64 = (static_cast<uint64_t>(p_large) * sc) >> 16;
    int32_t ref_pa = static_cast<int32_t>((ref_64 + 50U) / 100U);
    CHECK(qnh_large == ref_pa);

    if (g_test_failures == 0) {
        std::cout << "All test_baro_math tests passed!" << std::endl;
        return 0;
    } else {
        std::cerr << g_test_failures << " tests failed in test_baro_math!" << std::endl;
        return 1;
    }
}

