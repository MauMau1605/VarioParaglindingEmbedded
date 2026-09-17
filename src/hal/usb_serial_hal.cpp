/// @file usb_serial_hal.cpp
/// @brief USB CDC serial HAL implementation using Arduino Serial.

#include "hal/usb_serial_hal.hpp"
#include <Arduino.h>

namespace vario {
namespace hal {

/// Track initialization state without dynamic allocation.
static bool s_initialized = false;

Error usb_serial_init(const uint32_t baud_rate)
{
    Serial.begin(baud_rate);

    // On SAMD21 native USB, wait briefly for the host to enumerate.
    // Do not block indefinitely — the device must work even without a host.
    const uint32_t start = millis();
    while (!Serial) {
        if ((millis() - start) > 2000) {
            break;
        }
    }

    s_initialized = true;
    return Error::kOk;
}

Error usb_serial_write(const char* data, const uint8_t len)
{
    if (!s_initialized) {
        return Error::kNotInitialized;
    }

    if (data == nullptr || len == 0) {
        return Error::kInvalidData;
    }

    Serial.write(reinterpret_cast<const uint8_t*>(data), len);
    return Error::kOk;
}

bool usb_serial_is_connected()
{
    return s_initialized && static_cast<bool>(Serial);
}

}  // namespace hal
}  // namespace vario
