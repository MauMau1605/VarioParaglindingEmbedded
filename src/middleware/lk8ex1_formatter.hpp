/// @file lk8ex1_formatter.hpp
/// @brief LK8EX1 NMEA sentence formatter for variometer data.
///        Format: $LK8EX1,pressure,altitude,vario,temperature,battery,*checksum\r\n

#ifndef VARIOUSB_MIDDLEWARE_LK8EX1_FORMATTER_HPP
#define VARIOUSB_MIDDLEWARE_LK8EX1_FORMATTER_HPP

#include <cstdint>
#include "common/error_codes.hpp"
#include "common/types.hpp"

namespace vario {
namespace middleware {

/// @brief Format a VarioData struct into an LK8EX1 NMEA sentence.
/// @param data     Input: processed variometer data.
/// @param out_buf  Output buffer (caller-owned, must be >= buf_size bytes).
/// @param buf_size Size of the output buffer.
/// @param out_len  Output: number of bytes written (excluding null terminator).
/// @return Error::kOk on success, Error::kInvalidData if buffer too small.
Error lk8ex1_format(const VarioData* data,
                    char* out_buf,
                    uint8_t buf_size,
                    uint8_t* out_len);

}  // namespace middleware
}  // namespace vario

#endif  // VARIOUSB_MIDDLEWARE_LK8EX1_FORMATTER_HPP
