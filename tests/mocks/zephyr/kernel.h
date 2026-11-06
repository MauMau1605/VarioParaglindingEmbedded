#pragma once

#include <cstdint>

inline uint32_t g_mock_uptime_ms = 0;

inline uint32_t k_uptime_get_32() {
    return g_mock_uptime_ms;
}

inline void k_msleep(int32_t ms) {
    g_mock_uptime_ms += static_cast<uint32_t>(ms > 0 ? ms : 0);
}

inline void mock_set_uptime_ms(uint32_t ms) {
    g_mock_uptime_ms = ms;
}

inline void mock_advance_uptime_ms(uint32_t ms) {
    g_mock_uptime_ms += ms;
}

