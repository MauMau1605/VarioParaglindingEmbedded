/// @file i2c_hal.cpp
/// @brief I2C HAL implementation using Arduino Wire library.

#include "hal/i2c_hal.hpp"
#include <Wire.h>

namespace vario {
namespace hal {

Error i2c_init()
{
    Wire.begin();
    return Error::kOk;
}

Error i2c_read_register(const uint8_t device_addr,
                        const uint8_t reg_addr,
                        uint8_t* buffer,
                        const uint8_t len)
{
    if (buffer == nullptr || len == 0) {
        return Error::kInvalidData;
    }

    // Send register address
    Wire.beginTransmission(device_addr);
    Wire.write(reg_addr);
    const uint8_t tx_status = Wire.endTransmission(false);  // Repeated start

    if (tx_status != 0) {
        return (tx_status == 2) ? Error::kNack : Error::kTimeout;
    }

    // Request bytes from device
    const uint8_t received = Wire.requestFrom(device_addr, len);
    if (received != len) {
        return Error::kReadFailed;
    }

    for (uint8_t i = 0; i < len; ++i) {
        buffer[i] = static_cast<uint8_t>(Wire.read());
    }

    return Error::kOk;
}

Error i2c_write_register(const uint8_t device_addr,
                         const uint8_t reg_addr,
                         const uint8_t* data,
                         const uint8_t len)
{
    if (data == nullptr && len > 0) {
        return Error::kInvalidData;
    }

    Wire.beginTransmission(device_addr);
    Wire.write(reg_addr);

    for (uint8_t i = 0; i < len; ++i) {
        Wire.write(data[i]);
    }

    const uint8_t tx_status = Wire.endTransmission(true);

    if (tx_status != 0) {
        return (tx_status == 2) ? Error::kNack : Error::kWriteFailed;
    }

    return Error::kOk;
}

}  // namespace hal
}  // namespace vario
