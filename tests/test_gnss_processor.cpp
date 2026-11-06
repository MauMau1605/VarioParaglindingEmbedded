#include "check.hpp"
#include "middleware/gnss_processor.hpp"
#include "zephyr/kernel.h"
#include <iostream>

using namespace vario;
using namespace vario::middleware;

static void test_initial_state() {
    GnssProcessor proc;
    CHECK(proc.init() == Error::kOk);
    CHECK(proc.flight_state() == FlightState::kWaitGpsFix);
    CHECK_FALSE(proc.has_valid_fix());
    CHECK_EQ(proc.calibrated_qnh_pa(), config::kIsaSeaLevelPa);
}

static void test_state_transition_and_loss_of_fix() {
    GnssProcessor proc;
    proc.init();

    GnssFix fix = {};
    fix.valid = true;
    fix.ground_speed_mm_s = 500;
    fix.altitude_msl_mm = 500000;

    proc.on_fix(&fix);
    CHECK(proc.has_valid_fix());
    CHECK(proc.flight_state() == FlightState::kPreTakeoffCalib);

    // Invalidate fix
    fix.valid = false;
    proc.on_fix(&fix);
    CHECK_FALSE(proc.has_valid_fix());
    CHECK(proc.flight_state() == FlightState::kWaitGpsFix);
}

static void test_stationary_qnh_calibration() {
    GnssProcessor proc;
    proc.init();
    mock_set_uptime_ms(1000);

    // Altitude = 440 m (440000 mm), Pressure = 9600000 cPa (960 hPa)
    // Expected QNH ~ 101250 Pa
    GnssFix fix = {};
    fix.valid = true;
    fix.ground_speed_mm_s = 200;  // stationary (< 1500 mm/s)
    fix.altitude_msl_mm = 440000;

    // Feed baro and fixes for 4 seconds (< kQnhMinSamples = 5)
    for (int i = 0; i < 4; ++i) {
        mock_advance_uptime_ms(1000);
        for (int b = 0; b < 50; ++b) {
            proc.update_baro(9600000, 0);
        }
        proc.on_fix(&fix);
        CHECK_EQ(proc.calibrated_qnh_pa(), config::kIsaSeaLevelPa);
    }

    // 5th sample triggers calibration
    mock_advance_uptime_ms(1000);
    for (int b = 0; b < 50; ++b) {
        proc.update_baro(9600000, 0);
    }
    proc.on_fix(&fix);
    CHECK(proc.calibrated_qnh_pa() != config::kIsaSeaLevelPa);
    CHECK_NEAR(proc.calibrated_qnh_pa(), 101170, 50);

    // Feed non-stationary fix (speed = 2000 mm/s) -> calibration must NOT change
    int32_t saved_qnh = proc.calibrated_qnh_pa();
    mock_advance_uptime_ms(1000);
    fix.ground_speed_mm_s = 2000;
    fix.altitude_msl_mm = 600000; // should be ignored
    for (int b = 0; b < 50; ++b) {
        proc.update_baro(9000000, 0);
    }
    proc.on_fix(&fix);
    CHECK_EQ(proc.calibrated_qnh_pa(), saved_qnh);
}

static void test_ring_buffer_wraparound() {
    GnssProcessor proc;
    proc.init();
    mock_set_uptime_ms(1000);

    GnssFix fix = {};
    fix.valid = true;
    fix.ground_speed_mm_s = 100;
    fix.altitude_msl_mm = 500000;

    // Feed 25 samples (more than kQnhWindowSamples = 20)
    for (int i = 0; i < 25; ++i) {
        mock_advance_uptime_ms(1000);
        for (int b = 0; b < 50; ++b) {
            proc.update_baro(9540000, 0);
        }
        proc.on_fix(&fix);
    }
    CHECK(proc.calibrated_qnh_pa() > 100000);
}

static void test_takeoff_detection_high_vz() {
    GnssProcessor proc;
    proc.init();
    mock_set_uptime_ms(1000);

    GnssFix fix = {};
    fix.valid = true;
    fix.ground_speed_mm_s = 500;
    fix.altitude_msl_mm = 500000;
    proc.on_fix(&fix);
    CHECK(proc.flight_state() == FlightState::kPreTakeoffCalib);

    // Vz = 150 cm/s (> 120 cm/s threshold) for 2 seconds (< 3000 ms hold)
    proc.update_baro(9500000, 150);
    mock_advance_uptime_ms(1000);
    proc.on_fix(&fix);
    CHECK(proc.flight_state() == FlightState::kPreTakeoffCalib);

    mock_advance_uptime_ms(1000);
    proc.update_baro(9500000, 150);
    proc.on_fix(&fix);
    CHECK(proc.flight_state() == FlightState::kPreTakeoffCalib);

    // After 3rd second (total 3000 ms sustained) -> Takeoff!
    mock_advance_uptime_ms(1000);
    proc.update_baro(9500000, 150);
    proc.on_fix(&fix);
    CHECK(proc.flight_state() == FlightState::kInFlight);

    // In flight: temporary fix loss must NOT revert state
    fix.valid = false;
    proc.on_fix(&fix);
    CHECK(proc.flight_state() == FlightState::kInFlight);
}

static void test_takeoff_detection_speed_plus_vz() {
    GnssProcessor proc;
    proc.init();
    mock_set_uptime_ms(1000);

    GnssFix fix = {};
    fix.valid = true;
    fix.ground_speed_mm_s = 3000; // > 2500 mm/s
    fix.altitude_msl_mm = 500000;
    proc.on_fix(&fix);

    // Vz = 60 cm/s (> 50 cm/s with speed) for 3 seconds
    for (int i = 0; i < 3; ++i) {
        mock_advance_uptime_ms(1000);
        proc.update_baro(9500000, 60);
        proc.on_fix(&fix);
    }
    CHECK(proc.flight_state() == FlightState::kInFlight);
}

int main() {
    std::cout << "Running test_gnss_processor..." << std::endl;

    test_initial_state();
    test_state_transition_and_loss_of_fix();
    test_stationary_qnh_calibration();
    test_ring_buffer_wraparound();
    test_takeoff_detection_high_vz();
    test_takeoff_detection_speed_plus_vz();

    if (g_test_failures == 0) {
        std::cout << "All test_gnss_processor tests passed!" << std::endl;
        return 0;
    } else {
        std::cerr << g_test_failures << " tests failed in test_gnss_processor!" << std::endl;
        return 1;
    }
}
