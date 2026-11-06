/// @file buzzer.hpp
/// @brief Audio feedback engine for vario audio: climb beeps, sink alarm, one-shot tones.

#ifndef VARIO_MIDDLEWARE_BUZZER_HPP
#define VARIO_MIDDLEWARE_BUZZER_HPP

#include <cstdint>
#include "common/error_codes.hpp"

namespace vario {
namespace middleware {

class Buzzer {
public:
    Error init();

    /// @brief Update audio tone based on vertical speed (call at 50 Hz).
    /// @param climb_rate_cms Vertical speed in cm/s (+ climb, - sink).
    void update(int32_t climb_rate_cms);

    /// @brief Play a one-shot tone for system alerts or boot confirmation.
    /// @param freq_hz Tone frequency in Hertz.
    /// @param duration_ms Duration in milliseconds.
    void beep(uint32_t freq_hz, uint32_t duration_ms);

    /// @brief Stop all sound immediately and clear active states.
    void silence();

private:
    enum class State : uint8_t {
        kSilent,
        kClimb,
        kSink,
        kBeep,
    };

    State    m_state;
    uint32_t m_last_toggle_ms;
    uint32_t m_beep_end_ms;
    uint32_t m_current_freq_hz;
    bool     m_tone_on;

    uint32_t climb_rate_to_frequency(int32_t cms) const;
    uint32_t climb_rate_to_period(int32_t cms) const;
    void handle_climb_state(int32_t cms, uint32_t now);
    void handle_sink_state(int32_t cms);
};

}  // namespace middleware
}  // namespace vario

#endif  // VARIO_MIDDLEWARE_BUZZER_HPP

