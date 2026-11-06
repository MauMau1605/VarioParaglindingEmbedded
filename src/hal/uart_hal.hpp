/// @file uart_hal.hpp
/// @brief Hardware abstraction layer for SAM-M10Q UART communication.

#ifndef VARIO_HAL_UART_HAL_HPP
#define VARIO_HAL_UART_HAL_HPP

#include <cstdint>
#include "common/error_codes.hpp"

namespace vario::hal {

/// @brief Called from ISR context after bytes were pushed into the RX ring buffer.
///        MUST only signal (e.g. k_work_submit).
using UartRxNotify = void (*)(void* user_data);

Error    uart_init(UartRxNotify notify, void* user_data);
Error    uart_set_baud_rate(uint32_t baud);
Error    uart_send(const uint8_t* data, uint16_t len);
uint16_t uart_read(uint8_t* out, uint16_t max_len);
void     uart_flush_rx();
uint32_t uart_rx_overflow_count();

}  // namespace vario::hal

#endif  // VARIO_HAL_UART_HAL_HPP
