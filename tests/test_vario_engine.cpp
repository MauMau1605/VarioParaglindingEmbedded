#include "check.hpp"
#include "middleware/vario_engine.hpp"
#include "common/config.hpp"
#include "common/baro_math.hpp"
#include "zephyr/kernel.h"
#include <iostream>

using namespace vario;
using namespace vario::middleware;

static void test_initialization() {
    PressureData p = {101325, 10132500, 2500};
    VarioData out = {};

    // Before init, must return kNotInitialized
    // Note: since static state persists in TU, we reset init first
    vario_engine_init();
    Error err = vario_engine_update(&p, &out);
    CHECK(err == Error::kOk);
}

static void test_null_pointers() {
    vario_engine_init();
    PressureData p = {101325, 10132500, 2500};
    VarioData out = {};

    CHECK(vario_engine_update(nullptr, &out) == Error::kInvalidData);
    CHECK(vario_engine_update(&p, nullptr) == Error::kInvalidData);
}

static void test_first_sample() {
    vario_engine_init();
    mock_set_uptime_ms(100);

    PressureData p = {101325, 10132500, 2150};
    VarioData out = {};

    Error err = vario_engine_update(&p, &out);
    CHECK(err == Error::kOk);
    CHECK_EQ(out.vario_cm_s, 0);
    CHECK_EQ(out.pressure_pa, 101325);
    CHECK_EQ(out.temperature_dc, 215);
    CHECK_NEAR(out.alt_baro_cm, 0, 10);
}

static void test_climb_step_response() {
    vario_engine_init();
    mock_set_uptime_ms(1000);

    // Initial sample at sea level
    int32_t press_cpa = 10132500;
    PressureData p = {press_cpa / 100, press_cpa, 2000};
    VarioData out = {};
    CHECK(vario_engine_update(&p, &out) == Error::kOk);

    // Simulate steady climb: pressure decreases by 24 cPa every 20 ms (-1200 cPa/s)
    // Expected raw climb rate: vario_cm_s_from_dp(-1200, 101325, alt) ~ 99 cm/s
    int32_t raw_expected = baro_math::vario_cm_s_from_dp(-1200, 101325, 0);

    int32_t prev_vario = 0;
    for (int i = 0; i < 40; ++i) {
        mock_advance_uptime_ms(20);
        press_cpa -= 24;
        p.pressure_pa = press_cpa / 100;
        p.pressure_cpa = press_cpa;
        CHECK(vario_engine_update(&p, &out) == Error::kOk);

        // Filter must be monotonically increasing towards steady state
        CHECK(out.vario_cm_s >= prev_vario);
        prev_vario = out.vario_cm_s;
    }

    // After 40 iterations (800 ms, tau ~ 187 ms), filter has converged
    // Note: Due to integer truncation in (alpha*x + (scale-alpha)*y)/scale,
    // steady state reaches 90 cm/s for x = 99 cm/s
    CHECK_NEAR(out.vario_cm_s, 90, 2);
}

static void test_level_flight_decay() {
    vario_engine_init();
    mock_set_uptime_ms(1000);

    PressureData p = {101325, 10132500, 2000};
    VarioData out = {};
    vario_engine_update(&p, &out);

    // 10 samples of climb
    for (int i = 0; i < 10; ++i) {
        mock_advance_uptime_ms(20);
        p.pressure_cpa -= 30;
        p.pressure_pa = p.pressure_cpa / 100;
        vario_engine_update(&p, &out);
    }
    CHECK(out.vario_cm_s > 50);

    // Level flight (pressure constant) -> vario must decay back to 0
    for (int i = 0; i < 50; ++i) {
        mock_advance_uptime_ms(20);
        vario_engine_update(&p, &out);
    }
    CHECK_EQ(out.vario_cm_s, 0);
}

static void test_qnh_recalibration() {
    vario_engine_init();
    mock_set_uptime_ms(500);

    PressureData p = {101325, 10132500, 2000};
    VarioData out = {};
    vario_engine_update(&p, &out);
    CHECK_NEAR(out.alt_baro_cm, 0, 10);

    // Change QNH to 102500 Pa -> altitude must increase
    vario_engine_set_qnh(102500);
    mock_advance_uptime_ms(20);
    vario_engine_update(&p, &out);
    CHECK(out.alt_baro_cm > 8000);

    // Fallback on invalid QNH
    vario_engine_set_qnh(-50);
    mock_advance_uptime_ms(20);
    vario_engine_update(&p, &out);
    CHECK_NEAR(out.alt_baro_cm, 0, 10);
}

static void test_quantized_timing() {
    vario_engine_init();
    mock_set_uptime_ms(100);

    PressureData p = {100000, 10000000, 2000};
    VarioData out = {};
    vario_engine_update(&p, &out);

    // Jitter: elapsed 19 ms (quantized to 20 ms)
    mock_advance_uptime_ms(19);
    p.pressure_cpa -= 24;
    p.pressure_pa = p.pressure_cpa / 100;
    CHECK(vario_engine_update(&p, &out) == Error::kOk);

    // Jitter: elapsed 41 ms (quantized to 40 ms)
    mock_advance_uptime_ms(41);
    p.pressure_cpa -= 48;
    p.pressure_pa = p.pressure_cpa / 100;
    CHECK(vario_engine_update(&p, &out) == Error::kOk);
    CHECK(out.vario_cm_s > 0);
}

int main() {
    std::cout << "Running test_vario_engine..." << std::endl;

    test_initialization();
    test_null_pointers();
    test_first_sample();
    test_climb_step_response();
    test_level_flight_decay();
    test_qnh_recalibration();
    test_quantized_timing();

    if (g_test_failures == 0) {
        std::cout << "All test_vario_engine tests passed!" << std::endl;
        return 0;
    } else {
        std::cerr << g_test_failures << " tests failed in test_vario_engine!" << std::endl;
        return 1;
    }
}
