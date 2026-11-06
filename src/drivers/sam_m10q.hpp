/// @file sam_m10q.hpp
/// @brief Driver interface for the u-blox SAM-M10Q GNSS receiver over UBX binary protocol.

#ifndef VARIO_DRIVERS_SAM_M10Q_HPP
#define VARIO_DRIVERS_SAM_M10Q_HPP

#include <cstdint>
#include "common/error_codes.hpp"
#include "common/types.hpp"

namespace vario::gnss {

/// @brief Callback invoked when a validated GNSS fix is received and parsed.
using FixCallback = void (*)(const GnssFix* fix, void* user_data);

/// @brief Initialize SAM-M10Q receiver: perform cold/warm baud handshake,
///        send grouped UBX-CFG-VALSET configuration, and register fix callback.
/// @param cb Callback to receive validated fixes.
/// @param user_data User pointer passed to callback.
/// @return Error::kOk on success, Error::kTimeout on failure.
Error sam_m10q_init(FixCallback cb, void* user_data);

}  // namespace vario::gnss

#endif  // VARIO_DRIVERS_SAM_M10Q_HPP
