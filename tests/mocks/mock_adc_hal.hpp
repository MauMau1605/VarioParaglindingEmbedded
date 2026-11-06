#pragma once

#include "hal/adc_hal.hpp"
#include <cstdint>

namespace vario::hal {

struct MockAdcState {
    uint16_t battery_mv;
    Error return_status;
    uint32_t read_count;
};

extern MockAdcState g_mock_adc;

void mock_adc_reset();

}  // namespace vario::hal

