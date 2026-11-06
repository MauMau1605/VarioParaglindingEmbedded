/// @file buzzer.cpp
/// @brief Buzzer implementation for climb/sink acoustics using differential PWM.

#include "middleware/buzzer.hpp"
#include "hal/pwm_hal.hpp"
#include "common/config.hpp"
#include <zephyr/kernel.h>

namespace vario::middleware {

Error Buzzer::init()
{
    m_state = State::kSilent;
    m_last_toggle_ms = 0;
    m_beep_end_ms = 0;
    m_current_freq_hz = 0;
    m_tone_on = false;
    return hal::pwm_stop();
}

uint32_t Buzzer::climb_rate_to_frequency(int32_t cms) const
{
    if (cms < config::kBuzzerClimbMinCms) cms = config::kBuzzerClimbMinCms;
    if (cms > config::kBuzzerClimbMaxCms) cms = config::kBuzzerClimbMaxCms;
    const uint32_t delta_v = static_cast<uint32_t>(cms - config::kBuzzerClimbMinCms);
    const uint32_t range_v = static_cast<uint32_t>(config::kBuzzerClimbMaxCms - config::kBuzzerClimbMinCms);
    const uint32_t range_f = config::kBuzzerFreqClimbMaxHz - config::kBuzzerFreqClimbMinHz;
    return config::kBuzzerFreqClimbMinHz + (delta_v * range_f) / range_v;
}

uint32_t Buzzer::climb_rate_to_period(int32_t cms) const
{
    if (cms < config::kBuzzerClimbMinCms) cms = config::kBuzzerClimbMinCms;
    if (cms > config::kBuzzerClimbMaxCms) cms = config::kBuzzerClimbMaxCms;
    const uint32_t delta_v = static_cast<uint32_t>(cms - config::kBuzzerClimbMinCms);
    const uint32_t range_v = static_cast<uint32_t>(config::kBuzzerClimbMaxCms - config::kBuzzerClimbMinCms);
    const uint32_t range_p = config::kBuzzerPeriodClimbMaxMs - config::kBuzzerPeriodClimbMinMs;
    return config::kBuzzerPeriodClimbMaxMs - (delta_v * range_p) / range_v;
}

void Buzzer::handle_climb_state(const int32_t cms, const uint32_t now)
{
    const uint32_t period_ms = climb_rate_to_period(cms);
    const uint32_t on_time_ms = period_ms / 2;

    if (m_tone_on) {
        if ((now - m_last_toggle_ms) >= on_time_ms) {
            m_tone_on = false;
            m_current_freq_hz = 0;
            m_last_toggle_ms = now;
            (void)hal::pwm_stop();
        }
    } else {
        if ((now - m_last_toggle_ms) >= (period_ms - on_time_ms)) {
            m_tone_on = true;
            m_last_toggle_ms = now;
            const uint32_t freq = climb_rate_to_frequency(cms);
            m_current_freq_hz = freq;
            (void)hal::pwm_set_frequency(freq);
        }
    }
}

void Buzzer::handle_sink_state(const int32_t cms)
{
    int32_t sink = -cms;
    if (sink > 500) sink = 500;
    const uint32_t target_freq = config::kBuzzerFreqSinkHz - ((sink - 100) * 100) / 400;
    const uint32_t diff = (target_freq > m_current_freq_hz)
        ? (target_freq - m_current_freq_hz)
        : (m_current_freq_hz - target_freq);

    if (!m_tone_on || (diff * 50U >= m_current_freq_hz)) {
        (void)hal::pwm_set_frequency(target_freq);
        m_current_freq_hz = target_freq;
    }
    m_tone_on = true;
}

void Buzzer::silence()
{
    m_state = State::kSilent;
    m_tone_on = false;
    m_current_freq_hz = 0;
    (void)hal::pwm_stop();
}

void Buzzer::beep(const uint32_t freq_hz, const uint32_t duration_ms)
{
    m_state = State::kBeep;
    m_tone_on = true;
    m_current_freq_hz = freq_hz;
    m_beep_end_ms = k_uptime_get_32() + duration_ms;
    (void)hal::pwm_set_frequency(freq_hz);
}

void Buzzer::update(const int32_t climb_rate_cms)
{
    const uint32_t now = k_uptime_get_32();
    if (m_state == State::kBeep) {
        if (now >= m_beep_end_ms) silence();
        return;
    }
    if (climb_rate_cms >= config::kBuzzerClimbMinCms) {
        m_state = State::kClimb;
        handle_climb_state(climb_rate_cms, now);
    } else if (climb_rate_cms <= config::kBuzzerSinkThresholdCms) {
        m_state = State::kSink;
        handle_sink_state(climb_rate_cms);
    } else {
        silence();
    }
}

}  // namespace vario::middleware
