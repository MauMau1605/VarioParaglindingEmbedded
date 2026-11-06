#include "check.hpp"
#include "middleware/battery_monitor.hpp"
#include "mocks/mock_adc_hal.hpp"
#include <iostream>

using namespace vario;
using namespace vario::middleware;

static void test_initialization() {
    hal::mock_adc_reset();
    BatteryMonitor mon;
    CHECK(mon.init() == Error::kOk);
    CHECK_EQ(mon.voltage_mv(), 4000);
    CHECK_EQ(mon.percent(), 80);
}

static void test_lut_exact_points() {
    struct TestPoint {
        uint16_t mv;
        uint8_t expected_pct;
    };

    const TestPoint points[] = {
        {4300, 100}, // Clamped above max
        {4200, 100},
        {4150, 95},  // Midpoint 4100-4200
        {4100, 90},
        {4000, 80},  // Midpoint 3900-4100
        {3900, 70},
        {3850, 60},  // Midpoint 3800-3900
        {3800, 50},
        {3750, 40},  // Midpoint 3700-3800
        {3700, 30},
        {3650, 22},  // Midpoint 3600-3700: 15 + (50 * 15) / 100 = 22
        {3600, 15},
        {3550, 10},  // Midpoint 3500-3600: 5 + (50 * 10) / 100 = 10
        {3500, 5},
        {3250, 2},   // Midpoint 3000-3500: 0 + (250 * 5) / 500 = 2
        {3000, 0},
        {2800, 0}    // Clamped below min
    };

    for (const auto& tp : points) {
        hal::mock_adc_reset();
        hal::g_mock_adc.battery_mv = tp.mv;
        BatteryMonitor mon;
        mon.init();
        CHECK(mon.update() == Error::kOk);
        CHECK_EQ(mon.voltage_mv(), tp.mv);
        CHECK_EQ(mon.percent(), tp.expected_pct);
    }
}

static void test_ema_filter_convergence() {
    hal::mock_adc_reset();
    hal::g_mock_adc.battery_mv = 4000;
    BatteryMonitor mon;
    mon.init();

    // First sample takes measured voltage directly
    mon.update();
    CHECK_EQ(mon.voltage_mv(), 4000);

    // Sudden voltage drop to 3600 mV
    hal::g_mock_adc.battery_mv = 3600;

    // Step 1: (3*4000 + 3600)/4 = 3900
    mon.update();
    CHECK_EQ(mon.voltage_mv(), 3900);

    // Step 2: (3*3900 + 3600)/4 = 3825
    mon.update();
    CHECK_EQ(mon.voltage_mv(), 3825);

    // Step 3: (3*3825 + 3600)/4 = 3768
    mon.update();
    CHECK_EQ(mon.voltage_mv(), 3768);

    // After 20 updates, should fully converge to 3600
    for (int i = 0; i < 20; ++i) {
        mon.update();
    }
    CHECK_EQ(mon.voltage_mv(), 3600);
    CHECK_EQ(mon.percent(), 15);
}

static void test_adc_error_handling() {
    hal::mock_adc_reset();
    BatteryMonitor mon;
    mon.init();

    // Setup working reading
    hal::g_mock_adc.battery_mv = 3900;
    CHECK(mon.update() == Error::kOk);
    CHECK_EQ(mon.voltage_mv(), 3900);

    // Simulate hardware fault
    hal::g_mock_adc.return_status = Error::kHardwareFault;
    hal::g_mock_adc.battery_mv = 3000;
    CHECK(mon.update() == Error::kHardwareFault);

    // State must be preserved
    CHECK_EQ(mon.voltage_mv(), 3900);
}

int main() {
    std::cout << "Running test_battery_monitor..." << std::endl;

    test_initialization();
    test_lut_exact_points();
    test_ema_filter_convergence();
    test_adc_error_handling();

    if (g_test_failures == 0) {
        std::cout << "All test_battery_monitor tests passed!" << std::endl;
        return 0;
    } else {
        std::cerr << g_test_failures << " tests failed in test_battery_monitor!" << std::endl;
        return 1;
    }
}

