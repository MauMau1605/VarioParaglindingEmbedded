#include "tests/mocks/mock_adc_hal.hpp"

namespace vario::hal {

MockAdcState g_mock_adc = {4000, Error::kOk, 0};

void mock_adc_reset() {
    g_mock_adc.battery_mv = 4000;
    g_mock_adc.return_status = Error::kOk;
    g_mock_adc.read_count = 0;
}

Error adc_init() {
    return Error::kOk;
}

Error adc_read_battery_mv(uint16_t* battery_mv) {
    ++g_mock_adc.read_count;
    if (g_mock_adc.return_status != Error::kOk) {
        return g_mock_adc.return_status;
    }
    if (battery_mv != nullptr) {
        *battery_mv = g_mock_adc.battery_mv;
    }
    return Error::kOk;
}

}  // namespace vario::hal

