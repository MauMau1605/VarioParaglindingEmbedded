/// @file i2c_hal.cpp
/// @brief Zephyr I2C hardware abstraction implementation.

#include "hal/i2c_hal.hpp"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

namespace vario {
namespace hal {

static const struct device* const s_i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

static Error map_zephyr_err(const int err)
{
    if (err == 0) {
        return Error::kOk;
    }
    if (err == -ETIMEDOUT) {
        return Error::kTimeout;
    }
    if (err == -EIO) {
        return Error::kNack;
    }
    return Error::kReadFailed;
}

Error i2c_init()
{
    if (!device_is_ready(s_i2c_dev)) {
        return Error::kNotInitialized;
    }
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
    const int rc = i2c_write_read(s_i2c_dev, device_addr, &reg_addr, 1, buffer, len);
    return map_zephyr_err(rc);
}

Error i2c_write_register(const uint8_t device_addr,
                         const uint8_t reg_addr,
                         const uint8_t* data,
                         const uint8_t len)
{
    if (data == nullptr || len == 0) {
        return Error::kInvalidData;
    }
    const int rc = i2c_burst_write(s_i2c_dev, device_addr, reg_addr, data, len);
    return map_zephyr_err(rc);
}

}  // namespace hal
}  // namespace vario
