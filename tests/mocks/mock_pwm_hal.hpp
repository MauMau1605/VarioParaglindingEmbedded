#pragma once

#include "hal/pwm_hal.hpp"
#include <cstdint>

namespace vario::hal {

struct MockPwmState {
    uint32_t current_freq_hz;
    bool is_running;
    uint32_t set_freq_count;
    uint32_t stop_count;
};

extern MockPwmState g_mock_pwm;

void mock_pwm_reset();

}  // namespace vario::hal

