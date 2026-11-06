/// @file pwm_hal.cpp
/// @brief Zephyr PWM driver for differential piezo drive without DC bias.

#include "hal/pwm_hal.hpp"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>

namespace vario::hal {

static const struct device* const s_pwm_dev = DEVICE_DT_GET(DT_NODELABEL(pwm0));

namespace {
constexpr uint32_t kIdlePeriodNs = 1000000UL;  // only used if no tone was ever set
uint32_t s_period_ns = 0U;                      // 0 = never configured
bool     s_active    = false;

// pulse_ns == 0 -> both channels NORMAL polarity => both pins LOW.
static Error set_pair(uint32_t period_ns, uint32_t pulse_ns)
{
    const pwm_flags_t f1 = (pulse_ns == 0U) ? PWM_POLARITY_NORMAL : PWM_POLARITY_INVERTED;
    const int r0 = pwm_set(s_pwm_dev, 0, period_ns, pulse_ns, PWM_POLARITY_NORMAL);
    const int r1 = pwm_set(s_pwm_dev, 1, period_ns, pulse_ns, f1);
    return (r0 == 0 && r1 == 0) ? Error::kOk : Error::kPwmError;
}
}  // namespace

Error pwm_init()
{
    if (!device_is_ready(s_pwm_dev)) {
        return Error::kNotInitialized;
    }
    return pwm_stop();
}

Error pwm_set_frequency(uint32_t freq_hz)
{
    if (!device_is_ready(s_pwm_dev)) {
        return Error::kNotInitialized;
    }
    if (freq_hz == 0U) {
        return pwm_stop();
    }
    const uint32_t period_ns = 1000000000UL / freq_hz;
    if (s_active && period_ns != s_period_ns) {
        // Shared period: silence both channels on the CURRENT period first.
        const Error q = set_pair(s_period_ns, 0U);
        if (q != Error::kOk) {
            return q;
        }
    }
    const Error e = set_pair(period_ns, period_ns / 2U);  // 50% duty, ch1 inverted (180°)
    if (e == Error::kOk) {
        s_period_ns = period_ns;
        s_active = true;
    }
    return e;
}

Error pwm_stop()
{
    if (!device_is_ready(s_pwm_dev)) {
        return Error::kNotInitialized;
    }
    const uint32_t period_ns = (s_period_ns != 0U) ? s_period_ns : kIdlePeriodNs;
    const Error e = set_pair(period_ns, 0U);
    if (e == Error::kOk) {
        s_active = false;
    }
    return e;
}

}  // namespace vario::hal
