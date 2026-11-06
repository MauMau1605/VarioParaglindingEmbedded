/// @file sensor_bus_hal.cpp
/// @brief Sensor bus implementation delegating directly to I2C HAL.

#include "hal/sensor_bus_hal.hpp"
#include "hal/i2c_hal.hpp"
#include "common/config.hpp"

namespace vario {
namespace hal {

Error sensor_bus_init()
{
    return i2c_init();
}

Error sensor_bus_read_register(const uint8_t reg_addr,
                               uint8_t* buffer,
                               const uint8_t len)
{
    return i2c_read_register(config::kBmp390I2cAddress, reg_addr, buffer, len);
}

Error sensor_bus_write_register(const uint8_t reg_addr,
                                const uint8_t* data,
                                const uint8_t len)
{
    return i2c_write_register(config::kBmp390I2cAddress, reg_addr, data, len);
}

}  // namespace hal
}  // namespace vario
