/// @file sensor_bus_hal.cpp
/// @brief Unified sensor bus implementation delegating to I2C or SPI.

#include "hal/sensor_bus_hal.hpp"
#include "hal/i2c_hal.hpp"
#include "hal/spi_hal.hpp"
#include "common/config.hpp"

namespace vario {
namespace hal {

Error sensor_bus_init()
{
#if VARIO_USE_SPI
    return spi_init(config::kSpiCsPin);
#else
    return i2c_init();
#endif
}

Error sensor_bus_read_register(const uint8_t reg_addr,
                               uint8_t* buffer,
                               const uint8_t len)
{
#if VARIO_USE_SPI
    return spi_read_register(config::kSpiCsPin, reg_addr, buffer, len);
#else
    return i2c_read_register(config::kBmp390I2cAddress, reg_addr, buffer, len);
#endif
}

Error sensor_bus_write_register(const uint8_t reg_addr,
                                const uint8_t* data,
                                const uint8_t len)
{
#if VARIO_USE_SPI
    return spi_write_register(config::kSpiCsPin, reg_addr, data, len);
#else
    return i2c_write_register(config::kBmp390I2cAddress, reg_addr, data, len);
#endif
}

}  // namespace hal
}  // namespace vario
