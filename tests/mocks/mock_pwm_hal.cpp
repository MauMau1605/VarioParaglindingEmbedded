#include "tests/mocks/mock_pwm_hal.hpp"

namespace vario::hal {

MockPwmState g_mock_pwm = {0, false, 0, 0};

void mock_pwm_reset() {
    g_mock_pwm.current_freq_hz = 0;
    g_mock_pwm.is_running = false;
    g_mock_pwm.set_freq_count = 0;
    g_mock_pwm.stop_count = 0;
}

Error pwm_init() {
    mock_pwm_reset();
    return Error::kOk;
}

Error pwm_set_frequency(uint32_t freq_hz) {
    g_mock_pwm.current_freq_hz = freq_hz;
    g_mock_pwm.is_running = (freq_hz > 0);
    ++g_mock_pwm.set_freq_count;
    return Error::kOk;
}

Error pwm_stop() {
    g_mock_pwm.current_freq_hz = 0;
    g_mock_pwm.is_running = false;
    ++g_mock_pwm.stop_count;
    return Error::kOk;
}

}  // namespace vario::hal

