/// @file uart_hal.cpp
/// @brief Interrupt-driven UART HAL implementation with ring buffer for
/// SAM-M10Q.

#include "hal/uart_hal.hpp"
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

namespace vario::hal {

static const struct device *const s_uart_dev =
    DEVICE_DT_GET(DT_NODELABEL(uart1));

static struct ring_buf s_rx_rb;
static uint8_t s_rx_buf_storage[256];
static volatile uint32_t s_overflow = 0;
static UartRxNotify s_notify = nullptr;
static void *s_user = nullptr;

static void uart_isr(const struct device *dev, void *user_data) {
  ARG_UNUSED(user_data);
  uint8_t chunk[16];
  while (uart_irq_update(dev) && uart_irq_rx_ready(dev)) {
    const int n = uart_fifo_read(dev, chunk, sizeof(chunk));
    if (n <= 0) {
      break;
    }
    const uint32_t put_bytes =
        ring_buf_put(&s_rx_rb, chunk, static_cast<uint32_t>(n));
    if (put_bytes != static_cast<uint32_t>(n)) {
      ++s_overflow;
    }
    if (s_notify != nullptr) {
      s_notify(s_user);
    }
  }
}

Error uart_init(UartRxNotify notify, void *user_data) {
  if (!device_is_ready(s_uart_dev)) {
    return Error::kNotInitialized;
  }
  s_notify = notify;
  s_user = user_data;
  ring_buf_init(&s_rx_rb, sizeof(s_rx_buf_storage), s_rx_buf_storage);
  uart_irq_callback_user_data_set(s_uart_dev, uart_isr, nullptr);
  uart_irq_rx_enable(s_uart_dev);
  return Error::kOk;
}

Error uart_set_baud_rate(uint32_t baud) {
  if (!device_is_ready(s_uart_dev)) {
    return Error::kNotInitialized;
  }
  uart_irq_rx_disable(s_uart_dev);
  struct uart_config cfg{};
  if (uart_config_get(s_uart_dev, &cfg) != 0) {
    uart_irq_rx_enable(s_uart_dev);
    return Error::kUartError;
  }
  cfg.baudrate = baud;
  const int r = uart_configure(s_uart_dev, &cfg);
  ring_buf_reset(&s_rx_rb);
  uart_irq_rx_enable(s_uart_dev);
  return (r == 0) ? Error::kOk : Error::kUartError;
}

Error uart_send(const uint8_t *data, uint16_t len) {
  if (!device_is_ready(s_uart_dev)) {
    return Error::kNotInitialized;
  }
  if (data == nullptr || len == 0) {
    return Error::kInvalidData;
  }
  for (uint16_t i = 0; i < len; ++i) {
    uart_poll_out(s_uart_dev, data[i]);
  }
  return Error::kOk;
}

uint16_t uart_read(uint8_t *out, uint16_t max_len) {
  if (out == nullptr || max_len == 0) {
    return 0;
  }
  return static_cast<uint16_t>(ring_buf_get(&s_rx_rb, out, max_len));
}

void uart_flush_rx() { ring_buf_reset(&s_rx_rb); }

uint32_t uart_rx_overflow_count() { return s_overflow; }

} // namespace vario::hal
