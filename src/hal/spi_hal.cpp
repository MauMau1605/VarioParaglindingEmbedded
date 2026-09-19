/// @file spi_hal.cpp
/// @brief SPI HAL implementation using Arduino SPI library.

#include "hal/spi_hal.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <SPI.h>
#pragma GCC diagnostic pop

#include <Arduino.h>

namespace vario {
namespace hal {

/// Standard SPI settings for BMP390: 1 MHz, MSB first, SPI Mode 0.
static const SPISettings kSpiSettings(1000000UL, MSBFIRST, SPI_MODE0);

Error spi_init(const uint8_t cs_pin)
{
    pinMode(cs_pin, OUTPUT);
    digitalWrite(cs_pin, HIGH);

    SPI.begin();

    // Pulse CS low briefly to force BMP390 into SPI mode after power-up
    digitalWrite(cs_pin, LOW);
    delayMicroseconds(10);
    digitalWrite(cs_pin, HIGH);
    delay(2);

    return Error::kOk;
}

Error spi_read_register(const uint8_t cs_pin,
                        const uint8_t reg_addr,
                        uint8_t* buffer,
                        const uint8_t len)
{
    if (buffer == nullptr || len == 0) {
        return Error::kInvalidData;
    }

    SPI.beginTransaction(kSpiSettings);
    digitalWrite(cs_pin, LOW);

    // Send register address with Read bit (Bit 7 = 1)
    SPI.transfer(static_cast<uint8_t>(reg_addr | 0x80));

    // Bosch BMP390 requires 1 dummy byte on SPI reads
    SPI.transfer(0x00);

    for (uint8_t i = 0; i < len; ++i) {
        buffer[i] = SPI.transfer(0x00);
    }

    digitalWrite(cs_pin, HIGH);
    SPI.endTransaction();

    return Error::kOk;
}

Error spi_write_register(const uint8_t cs_pin,
                         const uint8_t reg_addr,
                         const uint8_t* data,
                         const uint8_t len)
{
    if (data == nullptr && len > 0) {
        return Error::kInvalidData;
    }

    SPI.beginTransaction(kSpiSettings);
    digitalWrite(cs_pin, LOW);

    // Send register address with Write bit (Bit 7 = 0)
    SPI.transfer(static_cast<uint8_t>(reg_addr & 0x7F));

    for (uint8_t i = 0; i < len; ++i) {
        SPI.transfer(data[i]);
    }

    digitalWrite(cs_pin, HIGH);
    SPI.endTransaction();

    return Error::kOk;
}

}  // namespace hal
}  // namespace vario
