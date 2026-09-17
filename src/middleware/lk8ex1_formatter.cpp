/// @file lk8ex1_formatter.cpp
/// @brief LK8EX1 NMEA sentence formatter implementation.
///        Produces: $LK8EX1,<press>,99999,<vario>,<temp>,999,*<checksum>\r\n

#include "middleware/lk8ex1_formatter.hpp"
#include "common/config.hpp"

#include <cstdio>   // snprintf
#include <cstring>  // strlen

namespace vario {
namespace middleware {

// ---------------------------------------------------------------------------
// Internal helper: compute NMEA XOR checksum between '$' and '*'
// ---------------------------------------------------------------------------
static uint8_t nmea_checksum(const char* sentence, const uint8_t len)
{
    uint8_t cs = 0;
    bool started = false;

    for (uint8_t i = 0; i < len; ++i) {
        if (sentence[i] == '$') {
            started = true;
            continue;
        }
        if (sentence[i] == '*') {
            break;
        }
        if (started) {
            cs ^= static_cast<uint8_t>(sentence[i]);
        }
    }

    return cs;
}

Error lk8ex1_format(const VarioData* data,
                    char* out_buf,
                    const uint8_t buf_size,
                    uint8_t* out_len)
{
    if (data == nullptr || out_buf == nullptr || out_len == nullptr) {
        return Error::kInvalidData;
    }

    // Minimum buffer size check (shortest possible sentence ~35 chars)
    if (buf_size < 40) {
        return Error::kInvalidData;
    }

    // Format the sentence body (without checksum)
    // $LK8EX1,pressure_Pa,altitude(99999),vario_cm_s,temp_dC/10,battery(999),*
    //
    // Temperature: VarioData stores decidegrees (215 = 21.5°C)
    // LK8EX1 expects integer °C or one decimal. We send integer part.
    const int16_t temp_int = data->temperature_dc / 10;

    const int written = snprintf(
        out_buf, buf_size,
        "$LK8EX1,%ld,%ld,%ld,%d,%d,*",
        static_cast<long>(data->pressure_pa),
        static_cast<long>(config::kAltitudeUnknown),
        static_cast<long>(data->vario_cm_s),
        static_cast<int>(temp_int),
        static_cast<int>(config::kBatteryUnknown));

    if (written <= 0 || written >= static_cast<int>(buf_size) - 5) {
        return Error::kInvalidData;
    }

    // Compute NMEA checksum
    const uint8_t cs = nmea_checksum(out_buf, static_cast<uint8_t>(written));

    // Append checksum as two hex digits + CRLF
    const int final_len = snprintf(
        out_buf + written, buf_size - written,
        "%02X\r\n",
        static_cast<unsigned int>(cs));

    if (final_len <= 0) {
        return Error::kInvalidData;
    }

    // The '*' was already included in the body, checksum follows it
    // Final result: $LK8EX1,101325,99999,15,21,999,*3A\r\n
    *out_len = static_cast<uint8_t>(written + final_len);

    return Error::kOk;
}

}  // namespace middleware
}  // namespace vario
