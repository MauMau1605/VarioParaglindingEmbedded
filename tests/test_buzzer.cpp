#include "check.hpp"
#include "middleware/buzzer.hpp"
#include "mocks/mock_pwm_hal.hpp"
#include "zephyr/kernel.h"
#include <iostream>

using namespace vario;
using namespace vario::middleware;

static void test_initialization() {
    hal::mock_pwm_reset();
    Buzzer b;
    CHECK(b.init() == Error::kOk);
    CHECK_FALSE(hal::g_mock_pwm.is_running);
    CHECK_EQ(hal::g_mock_pwm.current_freq_hz, 0);
}

static void test_deadband_silence() {
    hal::mock_pwm_reset();
    Buzzer b;
    b.init();

    // Rates in deadband: -100 < vz < +30
    const int32_t deadband_rates[] = {0, 10, 29, -1, -50, -99};
    for (int32_t r : deadband_rates) {
        b.update(r);
        CHECK_FALSE(hal::g_mock_pwm.is_running);
        CHECK_EQ(hal::g_mock_pwm.current_freq_hz, 0);
    }
}

static void test_climb_cadence_and_frequency() {
    hal::mock_pwm_reset();
    mock_set_uptime_ms(1000);
    Buzzer b;
    b.init();

    // Climb rate = 30 cm/s: period = 600 ms, on-time = 300 ms, freq = 700 Hz
    b.update(30);
    CHECK(hal::g_mock_pwm.is_running);
    CHECK_EQ(hal::g_mock_pwm.current_freq_hz, 700);

    // During on-time (e.g. +100 ms) -> still running
    mock_advance_uptime_ms(100);
    b.update(30);
    CHECK(hal::g_mock_pwm.is_running);

    // After on-time elapsed (+250 ms more = 350 ms > 300 ms) -> turned off
    mock_advance_uptime_ms(250);
    b.update(30);
    CHECK_FALSE(hal::g_mock_pwm.is_running);

    // After full period elapsed (+300 ms more = 650 ms > 600 ms) -> turned on again
    mock_advance_uptime_ms(300);
    b.update(30);
    CHECK(hal::g_mock_pwm.is_running);
    CHECK_EQ(hal::g_mock_pwm.current_freq_hz, 700);
}

static void test_strong_climb_frequency() {
    hal::mock_pwm_reset();
    mock_set_uptime_ms(2000);
    Buzzer b;
    b.init();

    // Rate = 500 cm/s: max frequency 2500 Hz
    b.update(500);
    CHECK(hal::g_mock_pwm.is_running);
    CHECK_EQ(hal::g_mock_pwm.current_freq_hz, 2500);

    // Rate > 500 cm/s (clamped to 2500 Hz)
    b.update(700);
    CHECK_EQ(hal::g_mock_pwm.current_freq_hz, 2500);
}

static void test_sink_alarm() {
    hal::mock_pwm_reset();
    mock_set_uptime_ms(3000);
    Buzzer b;
    b.init();

    // Rate = -100 cm/s: sink alarm starts at 300 Hz
    b.update(-100);
    CHECK(hal::g_mock_pwm.is_running);
    CHECK_EQ(hal::g_mock_pwm.current_freq_hz, 300);

    // Continuous tone: advancing time keeps it on
    mock_advance_uptime_ms(500);
    b.update(-100);
    CHECK(hal::g_mock_pwm.is_running);
    CHECK_EQ(hal::g_mock_pwm.current_freq_hz, 300);

    // Strong sink (-500 cm/s) -> frequency decreases to 200 Hz
    b.update(-500);
    CHECK(hal::g_mock_pwm.is_running);
    CHECK_EQ(hal::g_mock_pwm.current_freq_hz, 200);
}

static void test_one_shot_beep() {
    hal::mock_pwm_reset();
    mock_set_uptime_ms(4000);
    Buzzer b;
    b.init();

    // Start 1500 Hz beep for 100 ms
    b.beep(1500, 100);
    CHECK(hal::g_mock_pwm.is_running);
    CHECK_EQ(hal::g_mock_pwm.current_freq_hz, 1500);

    // During beep, climb rate updates are ignored
    mock_advance_uptime_ms(50);
    b.update(500);
    CHECK_EQ(hal::g_mock_pwm.current_freq_hz, 1500);

    // After duration expires (now >= 4100 ms) -> silenced
    mock_advance_uptime_ms(60);
    b.update(0);
    CHECK_FALSE(hal::g_mock_pwm.is_running);
}

int main() {
    std::cout << "Running test_buzzer..." << std::endl;

    test_initialization();
    test_deadband_silence();
    test_climb_cadence_and_frequency();
    test_strong_climb_frequency();
    test_sink_alarm();
    test_one_shot_beep();

    if (g_test_failures == 0) {
        std::cout << "All test_buzzer tests passed!" << std::endl;
        return 0;
    } else {
        std::cerr << g_test_failures << " tests failed in test_buzzer!" << std::endl;
        return 1;
    }
}

