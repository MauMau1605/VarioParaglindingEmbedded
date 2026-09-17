/// @file usb_serial_hal.hpp
/// @brief Hardware Abstraction Layer for USB CDC serial communication.
///        Wraps the Arduino Serial (native USB) to decouple upper layers.

#ifndef VARIOUSB_HAL_USB_SERIAL_HAL_HPP
#define VARIOUSB_HAL_USB_SERIAL_HAL_HPP

#include <cstdint>
#include "common/error_codes.hpp"

namespace vario {
namespace hal {

/// @brief Initialize the USB CDC serial port.
/// @param baud_rate Baud rate for the serial connection.
/// @return Error::kOk on success.
Error usb_serial_init(uint32_t baud_rate);

/// @brief Write a null-terminated string to USB serial.
/// @param data Pointer to the data buffer (null-terminated).
/// @param len  Number of bytes to write.
/// @return Error::kOk on success, Error::kNotInitialized if not started.
Error usb_serial_write(const char* data, uint8_t len);

/// @brief Check if a USB host is connected and the serial port is open.
/// @return true if connected and ready to receive data.
bool usb_serial_is_connected();

}  // namespace hal
}  // namespace vario

#endif  // VARIOUSB_HAL_USB_SERIAL_HAL_HPP
